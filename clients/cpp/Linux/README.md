Install dependencies:
sudo apt-get install libbluetooth-dev libglib2.0-dev libpcre3-dev cmake

Build and install Gattlib:
git clone https://github.com/labapart/gattlib.git
cd gattlib
mkdir build && cd build
cmake ..
make

Run ./scan to find your bt dongle and search for devices

