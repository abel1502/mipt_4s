# TracerGCC
This project implements the infrastructure to trace most of the basic operations on specific variables.
Its primary purpose is to help understand the intrinsic workings of compilers in terms of optimizations, as well as profile your code on a relatively high level.

## Installation
This project can be built via CMake:
```bash
mkdir build
cd build
cmake ..
make
```
Please note that the binary relies upon residing in and being started
from the `build` subdirectory.

## Usage
To use the features of this project, include `tracer.h` and `trace.h`.
To trace operations over specific variables, declare them either via
the `DECL_TVAR(T, NAME, [VALUE])` macro, or just by creating a
`Tracer<T>` instance. (Note that the first approach also remembers the 
variable's name). To add function (scope) information, use the `FUNC_GUARD` macro, or just instantiate a `TraceFuncGuard` inside the
scope.

The results are being accumulated in the global `Trace` instance, available via `Trace::getInstance()`. To represent them nicely, you may use on of the visualizers in `./visualizers` - currently, dot (GraphViz) and HTML are available. You may see examples of their usage and results below.

## Examples
TODO
