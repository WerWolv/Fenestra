FROM emscripten/emsdk:3.1.74 as build

# Used to invalidate layer cache but not mount cache
# See https://github.com/moby/moby/issues/41715#issuecomment-733976493
ARG UNIQUEKEY 1

RUN apt update
RUN apt install -y git ccache autoconf automake libtool cmake pkg-config ninja-build

RUN <<EOF
# Install vcpkg
# Note: we are a patch on the libmagic port
set -xe

git clone https://github.com/microsoft/vcpkg /vcpkg
/vcpkg/bootstrap-vcpkg.sh
EOF

# Patch vcpkg build instructions to add -pthread
RUN <<EOF
set -xe

echo '
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
' >> /emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
EOF

ENV VCPKG_DEFAULT_BINARY_CACHE /cache/vcpkg
RUN --mount=type=cache,target=/cache <<EOF
# Install dependencies with vcpkg
set -xe

mkdir -p $VCPKG_DEFAULT_BINARY_CACHE

/vcpkg/vcpkg install --triplet=wasm32-emscripten freetype
EOF

# Build Fenestra
ARG JOBS=4
ENV CCACHE_DIR /cache/ccache

RUN mkdir /build
WORKDIR /build
RUN --mount=type=cache,target=/cache \
    --mount=type=bind,source=.,target=/fenestra <<EOF

set -xe
ccache -zs

cmake /fenestra                                                                                         \
    -G "Ninja"                                                                                          \
    -DFENESTRA_STATIC_LINK_PLUGINS=ON                                                                   \
    -DFENESTRA_COMPRESS_DEBUG_INFO=OFF                                                                  \
    -DNATIVE_CMAKE_C_COMPILER=gcc                                                                       \
    -DNATIVE_CMAKE_CXX_COMPILER=g++                                                                     \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache                                                                  \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache                                                                \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake                                      \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DCMAKE_BUILD_TYPE=Release

ninja -j $JOBS

cp /fenestra/dist/web/source/* /build
ccache -s
EOF

# Create a file dedicated to store wasm size, because I know no way to get the wasm content length if the web server uses compression
# See https://stackoverflow.com/questions/41701849/cannot-modify-accept-encoding-with-fetch https://github.com/AnthumChris/fetch-progress-indicators/issues/13
RUN du -b /build/fenestra.wasm | cut -f1 > fenestra.wasm.size

FROM scratch as raw
COPY --from=build [                 \
    # Fenestra                      \
    "/build/fenestra.wasm",         \
    "/build/fenestra.wasm.size",    \
    "/build/fenestra.js",           \
    "/build/fenestra.worker.js",    \
                                    \
    # Static files                  \
    "/build/index.html",            \
    "/build/style.css",             \
    "/build/wasm-config.js",        \
    "/build/enable-threads.js",     \
                                    \
    # Destination                   \
    "./"                            \
]

FROM nginx
COPY --from=raw . /usr/share/nginx/html
