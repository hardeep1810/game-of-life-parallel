# ---------- Build Stage ----------
FROM debian:bookworm-slim AS build

WORKDIR /src

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    git \
    pkg-config \
    libopenmpi-dev \
    openmpi-bin \
    libeigen3-dev \
    libx11-dev libxcursor-dev libxi-dev libxrandr-dev libudev-dev \
    libfreetype6-dev libglew-dev libjpeg-dev libsndfile1-dev libopenal-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy project source (excluding build artifacts via .dockerignore)
COPY . /src

# Build project using CMake
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --target game_of_life -j$(nproc)

# Install binary for runtime stage
RUN cp build/game_of_life /usr/local/bin/game_of_life

# ---------- Runtime Stage ----------
FROM debian:bookworm-slim AS run

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    openmpi-bin \
    libeigen3-dev \
    libx11-6 libxrandr2 libudev1 libfreetype6 libglew2.2 \
    libjpeg62-turbo libsndfile1 libopenal1 libxi6 libxcursor1 \
    && rm -rf /var/lib/apt/lists/*

# Copy the built binary
COPY --from=build /usr/local/bin/game_of_life /usr/local/bin/game_of_life

# Set working directory (e.g., for input/output data)
WORKDIR /data

# Set MPI and library environment
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
ENV GOL_NUM_MPI_PROCS=2
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

ENTRYPOINT ["sh", "-c", "mpirun --allow-run-as-root --oversubscribe -n ${GOL_NUM_MPI_PROCS} /usr/local/bin/game_of_life \"$@\"", "--"]
CMD ["-n", "-r", "128", "-c", "128", "-i", "100"]
