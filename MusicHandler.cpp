#include "MusicHandler.h"

#include <mpg123.h>
#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>

#include <iostream>
#include <memory>

MusicHandler::MusicHandler()
{
    this->shutdown = false;
    this->playing = false;
    this->streaming = false;

    // Initialize MP3 library
    mpg123_init();

    // Initialize PipeWire
    pw_init(nullptr, nullptr);

    this->playbackThread = std::thread(&MusicHandler::playMusic, this);
    position = 0;
    index = 0;
}

MusicHandler::~MusicHandler()
{
    this->shutdown = true;
    cv.notify_one();
    if (this->playbackThread.joinable())
    {
        this->playbackThread.join();
    }
    mpg123_exit();
    pw_deinit();
}

void MusicHandler::setContent(std::vector<std::string> filenames)
{
    this->playlist = filenames;
    this->index = 0;
    this->position = 0;
}

void MusicHandler::startPlay()
{
    this->playing = true;
    this->streaming = true;
    cv.notify_one();
}

void MusicHandler::stopPlay()
{
    if(!this->streaming) {
        return;
    }
    this->streaming = false;
    playing.wait(true);
}

bool ends_with(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void MusicHandler::backward()
{
    bool wasStreaming = this->streaming;
    if(this->streaming) {
        this->stopPlay();
    }
    if (index > 0)
    {
        index--;
    }
    else
    {
        index = 0;
    }
    position = 0;
    if(wasStreaming) {
        this->startPlay();
    }
}

void MusicHandler::forward()
{
    bool wasStreaming = this->streaming;
    if(this->streaming) {
        this->stopPlay();
    }
    if (index < (playlist.size() - 1))
    {
        index++;
    }
    else
    {
        index = 0;
    }
    position = 0;
    if(wasStreaming) {
        this->startPlay();
    }    
}

long MusicHandler::getPosition()
{
    return position;
}

int MusicHandler::getIndex()
{
    return index;
}

void MusicHandler::setPosition(long position)
{
    this->position = position;
}

void MusicHandler::setIndex(int index)
{
    this->index = index;
}

void MusicHandler::seek(int seconds)
{
    this->stopPlay();
    this->position = this->position + seconds;
    this->startPlay();
}

long MusicHandler::getLength(int index)
{
    mpg123_handle *mp3 = mpg123_new(nullptr, nullptr);
    mpg123_open(mp3, this->playlist[index].c_str());
    int channels;
    int encoding;
    long rate;
    mpg123_getformat(mp3, &rate, &channels, &encoding);
    long length = mpg123_length(mp3);
    mpg123_close(mp3);
    mpg123_delete(mp3);
    return length/rate;
}

void MusicHandler::seekInPlaylist(int seconds)
{
    this->stopPlay();
    long songLength = this->getLength(this->index);
   
    int newPosition = this->position + seconds;
    if (newPosition < 0)
    {
        if (index > 0)
        {
            this->index--;
            this->position = this->getLength(this->index) + newPosition;
        }
        else
        {
            this->position = 0;
        }
    }
    else if (newPosition >= songLength)
    {
        if (index < (playlist.size() - 1))
        {
            index++;
            this->position = newPosition-songLength;
        }
        else
        {
            this->position = songLength;
        }
    }
    else
    {
        this->position = newPosition;
    }
    this->startPlay();
}

struct AudioData
{
    pw_main_loop *loop;
    pw_stream *stream;
    mpg123_handle *mp3;
    bool done;
};

static void on_process(void *userdata)
{
    AudioData *data = static_cast<AudioData *>(userdata);
    pw_buffer *b = pw_stream_dequeue_buffer(data->stream);
    if (!b)
    {
        return;
    }

    spa_buffer *buf = b->buffer;
    int16_t *dst = static_cast<int16_t *>(buf->datas[0].data);
    if (!dst)
    {
        return;
    }

    size_t samples = buf->datas[0].maxsize / sizeof(int16_t);
    size_t done = 0;
    int err = mpg123_read(data->mp3,
                          reinterpret_cast<unsigned char *>(dst),
                          samples * sizeof(int16_t),
                          &done);

    if (err == MPG123_DONE)
    {
        data->done = true;
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = sizeof(int16_t);
    buf->datas[0].chunk->size = done;

    pw_stream_queue_buffer(data->stream, b);
}

static const pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

void MusicHandler::playMusic()
{

    AudioData data = {nullptr, nullptr, nullptr, false};

    while (!this->shutdown)
    {
        if (!playing)
        {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock);
        }
        if(this->playlist.size() == 0) {
            this->playing = false;
            this->playing.notify_one();
            continue;
        }
        data.mp3 = mpg123_new(nullptr, nullptr);
        data.done = false;
        mpg123_open(data.mp3, this->playlist[index].c_str());

        int channels;
        int encoding;
        long rate;
        mpg123_getformat(data.mp3, &rate, &channels, &encoding);

        data.loop = pw_main_loop_new(nullptr);
        if (!data.loop)
        {
            std::cerr << "Failed to create main loop" << std::endl;
            return;
        }

        uint8_t buffer[1024];
        spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        u_int32_t channelsSPA = channels;
        u_int32_t rateSPA = rate;
        const struct spa_pod *params[1];
        spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
                .format = SPA_AUDIO_FORMAT_S16,
                .rate = rateSPA,
                .channels = channelsSPA);
        params[0] = spa_format_audio_raw_build(&b,
                                               SPA_PARAM_EnumFormat,
                                               &info);

        data.stream = pw_stream_new_simple(
            pw_main_loop_get_loop(data.loop),
            "mp3-player",
            pw_properties_new(
                PW_KEY_MEDIA_TYPE, "Audio",
                PW_KEY_MEDIA_CATEGORY, "Playback",
                PW_KEY_MEDIA_ROLE, "Music",
                nullptr),
            &stream_events,
            &data);

        if (!data.stream)
        {
            std::cerr << "Failed to create stream" << std::endl;
            return;
        }
        enum pw_stream_flags flags = (enum pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS);
        pw_stream_connect(data.stream,
                          PW_DIRECTION_OUTPUT,
                          PW_ID_ANY,
                          flags,
                          params, 1);

        if (this->position > 0)
        {
            mpg123_seek(data.mp3, this->position * rate, SEEK_SET);
        }

        struct pw_loop *loop = pw_main_loop_get_loop(data.loop);
        while (this->streaming)
        {
            int result = pw_loop_iterate(loop, 100);            
            if(result < 0 || data.done == true) {
                break;
            }
        }

        pw_stream_destroy(data.stream);
        data.stream = nullptr;

        this->position = mpg123_tell(data.mp3) / rate;

        mpg123_close(data.mp3);
        mpg123_delete(data.mp3);

        if (index < (playlist.size() - 1) && this->streaming)
        {
            this->index++;
            this->position = 0;
        }
        else
        {
            // reset playstate when finished
            if (this->streaming)
            {
                this->index = 0;
                this->position = 0;
                this->streaming = false;
            }
            this->playing = false;
            this->playing.notify_one();
        }
    }
}