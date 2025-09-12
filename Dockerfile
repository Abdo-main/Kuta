# Build stage
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libglfw3-dev \
    libvulkan-dev \
    libcglm-dev \
    libassimp-dev \
    vulkan-tools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN git submodule update --init --recursive && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel $(nproc)

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libglfw3 \
    libvulkan1 \
    libcglm0 \
    libassimp5 \
    vulkan-tools \
    mesa-vulkan-drivers \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /workspace/build/kuta .
COPY --from=builder /workspace/build/*.spv .

# Set environment for headless Vulkan
ENV VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json
ENV VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d

CMD ["./kuta"]
