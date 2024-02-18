# glados-tts.cpp

A C++ implementation of [glados-tts by nerdaxic](https://github.com/nerdaxic/glados-tts) using LibTorch.

## Installation

Grab a copy of LibTorch from [their webiste](https://pytorch.org/get-started/locally/) and install [espeak-ng](https://github.com/espeak-ng/espeak-ng).

Then, run the build script:

    ./build_linux.sh [/path/to/libtorch]

replacing "/path/to/libtorch" with the absolute path to the libtorch installation folder.

## Usage

    cd build
    ./glados-tts "text to read"