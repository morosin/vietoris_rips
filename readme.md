Requirements
============
This project relies on experimental C++26 features, namely [P2996 (Reflection)](https://wg21.link/p2996), that are not yet available in most compilers. It can be compiled with the [P2996 Clang fork](https://github.com/bloomberg/clang-p2996/tree/p2996), and possibly with a snapshot build of [GCC 16](https://gcc.gnu.org/pub/gcc/snapshots/LATEST-16/), but I can only attest to the former. 

The project relies on [Eigen](https://libeigen.gitlab.io/) for its linear algebra computations, which is included as a git submodule. Run `git submodule update` before building. Otherwise, this can be build like normal with CMake, by running `cmake -S . -B build/` followed by `cmake --build build/`. 

In order to display the output correctly, this project prints ANSI escape codes to the terminal. While it should be compatible with any standards-compliant terminal emulator (e.g. [xterm](https://invisible-island.net/xterm/manpage/xterm.html), [kitty](https://sw.kovidgoyal.net/kitty/)), I can't promise it'll work on Windows. It also wants a terminal that's at least 110 columns wide, so unfortunately it may not work with anyone's VT100. 

Example
=======
See [here](./demo.pdf) for an example output.

