module load gcc
mkdir build
cd build
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_FLAGS="-I/l/gcc-4.7.2/include/c++/4.7.2/x86_64-unknown-linux-gnu -I/l/gcc-4.7.2/include/c++/4.7.2" -DCMAKE_EXE_LINKER_FLAGS="-L/l/gcc-4.7.2/lib64" ..
make -j8