# NOTE

**I am currently rewriting this to use modern C++ and best practices. So this README is currently out of date.**

# Purpose:

NumPy offers the `save` method for easy saving of arrays into .npy and `savez` for zipping multiple .npy arrays together
into a .npz file.

`cnpy` lets you read and write to these formats in C++.

The motivation comes from scientific programming where large amounts of data are generated in C++ and analyzed in
Python.

Writing to .npy has the advantage of using low-level C++ I/O (fread and fwrite) for speed and binary format for size.
The .npy file header takes care of specifying the size, shape, and data type of the array, so specifying the format of
the data is unnecessary.

Loading data written in numpy formats into C++ is equally simple, but requires you to type-cast the loaded data to the
type of your choice.

# Installation:

There are two ways to use the library:

## 1. Using CMake:

Just include this into your `CMakeLists.txt`.
```cmake
include(FetchContent)
FetchContent_Declare(cnpy
        GIT_REPOSITORY "https://github.com/TomSchammo/cnpy"
        GIT_SHALLOW True
)
FetchContent_MakeAvailable(cnpy)
```

This library needs support for C++20 features, so at least GCC 13 and setting the `CMAKE_CXX_STANDARD` to 20 or above are required.

## 2. By installing the library on your system:

1. Install [cmake](www.cmake.org) and [git](https://git-scm.com/)
2. Run `git clone https://github.com/TomSchammo/cnpy` and `cd cnpy`
3. Create a build directory and change into it using `mkdir build` and `cd build`
4. Run CMake: `cmake ..`. This library uses the new dual ABI by default. If you need support for the legacy ABI, don't forget to set the flags using `-DCMAKE_CXX_FLAGS="-D_GLIBCXX_USE_CXX11_ABI=0"`
5. Run `make` to build and `make install` to install the project.

# Using:

To use, `#include"cnpy.h"` in your source code. Compile the source code mycode.cpp as

```bash
g++ -o mycode mycode.cpp -L/path/to/install/dir -lcnpy -lz --std=c++11
```

# Description:

There are two functions for writing data: `npy_save` and `npz_save`.

There are 3 functions for reading:

- `npy_load` will load a .npy file.
- `npz_load(fname)` will load a .npz and return a dictionary of NpyArray structures.
- `npz_load(fname,varname)` will load and return the NpyArray for data varname from the specified .npz file.

The data structure for loaded data is below.
Data is accessed via the `data<T>()`-method, which returns a pointer of the specified type (which must match the
underlying datatype of the data).
The array shape and word size are read from the npy header.

```c++
struct NpyArray {
    std::vector<size_t> shape;
    size_t word_size;
    template<typename T> T* data();
};
```

See [example1.cpp](test/example1.cpp) for examples of how to use the library. example1 will also be build during cmake
installation.
