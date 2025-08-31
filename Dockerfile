FROM debian:12

# Install build deps
RUN set -eux; \
    apt-get update; \
    apt-get install -y \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        git \
        ca-certificates \
        libsdl2-dev \
        xvfb \
        xauth; \
    update-ca-certificates; \
    rm -rf /var/lib/apt/lists/*

# Set workdir inside container
WORKDIR /workspace

# Default command just drops into bash
CMD ["/bin/bash"]

