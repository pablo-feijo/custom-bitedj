ARG BASE_IMAGE=debian:trixie
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive
ENV CCACHE_DIR=/root/.cache/ccache

# Install minimal base utilities needed for the setup script
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    ca-certificates \
    git \
    sudo \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Install all BiteDJ build dependencies using the upstream setup script
COPY tools/debian_buildenv.sh /tmp/debian_buildenv.sh
RUN /tmp/debian_buildenv.sh setup < /dev/null \
    && rm -rf /var/lib/apt/lists/* /tmp/debian_buildenv.sh

# Allow git to operate inside container mounts without dubious ownership warnings
RUN git config --global --add safe.directory '*'

WORKDIR /src
