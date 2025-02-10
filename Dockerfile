# Dockerfile
FROM debian:bookworm

# Install dependencies
RUN dpkg --add-architecture arm64 && \
    apt-get update && apt-get install -y \
    crossbuild-essential-arm64 \
    cmake \
    libcap2 libcap-dev \
    pkg-config \
    libboost-all-dev \
    libcap-dev:arm64 \
    libmpg123-dev:arm64 \
    libpipewire-0.3-dev:arm64 \
    wget \
    git

# Download and compile bcm2835 library
RUN wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.75.tar.gz && \
    tar -xvf bcm2835-1.75.tar.gz && \
    cd bcm2835-1.75 && \
    CFLAGS="-DBCM2835_HAVE_LIBCAP" ./configure --host=aarch64-linux-gnu --prefix=/usr/aarch64-linux-gnu && \
    make && \
    make install

# Set up the cross-compiler environment
COPY PiZero.cmake /PiZero.cmake