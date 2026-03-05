# RC Car
An RC car project using the Raspberry Pi Pico SDK.
## Setup

Clone the raspberry pico sdk here:
```sh
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
```

Next install dependencies (this may vary depending on linux mac etc...):
```sh
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential 
```

make sure the `pico-sdk` is in the directory of the rc-car project.

The main cmake file is located at `rc-car/CMakeLists.txt`. You should add dependencies and anything else there.


## Building
```sh
cmake -S . -B build
cd build
make -j4
```

Will produce a binary named `main.uf2` which can be flashed to the Pico.
