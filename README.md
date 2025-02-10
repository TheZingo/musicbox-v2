
# Music Box v2

The Music Box is a project that allows you to control a music player using RFID tags. By placing figures on the box, you can start playing the associated song, playlist or audio book. The playback is automatically stopped when removing the figure, providing a toddler-proof interface. You can find more information in the following article: [Please tell me more about Music Box v2.0](https://ptmm.de/about/musicbox-v2/)


## Features

* Control music playback using RFID-tagged figures
* Automatically start and stop playback when adding or removing figures
* Use the default pipewire device for playback (e.g. an attached soundcard)
* Configurable through a local web interface
* Supports custom 3D printed figures



## Development

On Windows use WSL2 with Debian Bookworm (Microsoft Store), the Dockerfile in this repository or a full Debian Bookworm.

### Install cross-compiler

To set up the cross-compiler, run the following commands:

```bash
sudo apt update
sudo apt install crossbuild-essential-arm64 cmake
sudo apt install libcap2 libcap-dev
sudo apt install pkg-config
sudo apt install libboost-all-dev
```

### Enable Multi-arch Support

Enable multi-architecture support with the following commands:

```bash
sudo dpkg --add-architecture arm64
sudo apt update
```

### Install libcap
For the RFID reader, access to `/dev/mem` is required which is restricted to the root account.
With libcap and a compiler flag for bcm2835 the access can be enabled for non-root users.
```bash
sudo apt install libcap-dev:arm64
```

### Download and compile bcm2835 library

Download and compile the bcm2835 library:

```bash
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.75.tar.gz
tar -xvf bcm2835-1.75.tar.gz
cd bcm2835-1.75
CFLAGS="-DBCM2835_HAVE_LIBCAP" ./configure --host=aarch64-linux-gnu --prefix=/usr/aarch64-linux-gnu
make
sudo make install
```

### Install mpg123

Install the mpg123 library:

```bash
sudo apt install libmpg123-dev:arm64
```

### Install pipewire-dev

Install the pipewire development library

```bash
sudo apt install libpipewire-0.3-dev:arm64
```

## Deployment

Prerequisite: 
* Having a user "music" 

For powering off the Pi Zero when the battery is low, the user needs permission to execute `sudo poweroff` without a password. Add the following line to `/etc/sudoers`: `music ALL=(ALL) NOPASSWD: ALL` or `music ALL=NOPASSWD: /usr/sbin/poweroff`.

```bash
sudo apt update
sudo apt install pipewire
sudo usermod -aG audio music
```

Update `/boot/firmware/config.txt` to activates mini amp support and deactivates the led of board as well as HDMI out to save power.
```ini
# Activate hifi berry mini amp support
dtoverlay=hifiberry-dac

# Disable the ACT LED on the Pi Zero
dtparam=act_led_trigger=none
dtparam=act_led_activelow=on

# Disable HDMI  
hdmi_blanking=2  
enable_tvout=0 
```
Add the following udev rule to grant access to `/dev/mem` for users in the `kmem` group:

```bash
echo 'SUBSYSTEM=="mem", KERNEL=="mem", GROUP="kmem", MODE="0660"' | sudo tee /etc/udev/rules.d/98-mem.rules
sudo adduser $USER kmem
sudo reboot
```

Grant the executable CAP_SYS_RAWIO capability to perform I/O operations in user context
```bash
sudo setcap cap_sys_rawio+ep MusicBox
```

Enable auto start of the app in user mode
```sh
loginctl enable-linger music
mkdir -p ~/.config/systemd/user
cp ~/docs/musicbox.service ~/.config/systemd/user/
systemctl --user enable musicbox.service
```

## Web Frontend

Prerequisites
* Node.js and npm installed on your system


### Project setup
```sh
npm install
```

### Deploy
To build the web root call the following command in ```/webroot```
```sh
npm run build
```

copy ```/webroot/dist``` to the Raspberry Pi ```~/webroot```


## API

The REST API is mainly used by the web interface to communicate with the media player. 
It can also be used to build a native app or in a smart home setup to act on certain tags.

| Method   | URL                                  | Description                                                   |
| -------- | ------------------------------------ | ------------------------------------------------------------- |
| `GET`    | `/api/state`                         | Get the currently active tag and play state                   |
| `GET`    | `/api/content/list`                  | Get all content elements stored on the box                    |
| `POST`   | `/api/content/reload`                | Reload content directory                                      |
| `GET`    | `/api/content/<id>`                  | Get full details for the given content element                |
| `GET`    | `/api/content/<id>/cover`            | Download the cover image for the given content element        |
| `GET`    | `/api/tags`                          | Get the list of all managed tags                              |
| `GET`    | `/api/tags/<id>`                     | Get tag details                                               |
| `POST`   | `/api/tags/<id>`                     | Update tag details (like what to play)                        |
| `DELETE` | `/api/tags/<id>`                     | Remove tag data                                               |
| `POST`   | `/api/tags/<id>/reset`               | Reset play state (current index of the playlist and song position) |
| `POST`   | `/api/content/upload`                | File upload endpoint                                          |
| `WS`     | `/api/live`                          | Websocket endpoint to get live state updates                  |


## Update the application

1. Stop the service
   ```sh
   systemctl --user stop musicbox
   ```
2. Copy the new executable into the user directory of user "music"
3. Grant the executable CAP_SYS_RAWIO capability to perform I/O operations in user context
   ```sh
   sudo setcap cap_sys_rawio+ep MusicBox
   ```
4. Start the service
   ```sh
   systemctl --user start musicbox

## Contributing
If you would like to contribute to the Music Box project, please feel free to submit issues or pull requests on the GitHub repository. Contributions are welcome for features, bug fixes, documentation, and more.


## Thanks 
MusicBox is built using great open-source projects including ....

* RPi-RFID
* BCM2835
* rpi_ws281x 
* CrowCpp
* nlohmann_json
* libmpg123-0 
* libcap2
* pipewire