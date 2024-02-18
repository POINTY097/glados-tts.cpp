mkdir build
cd build
TORCH_DIR=$1
cmake -DTORCH_DIR="${TORCH_DIR}" ..
cmake --build . --config Debug