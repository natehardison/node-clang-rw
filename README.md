Overview
========
This Node module allows you to do simple source code rewriting with Clang.

More info is forthcoming!


TODOs
=====
* On Fedora 17, the installed version of clang can find the standard libraries,
  but the rewriter can't. This should be fixable by setting the
  `HeaderSearchOptions`, but I can't seem to get it.


Gotchas
=======
* On Fedora, clang++ 3.0 doesn't handle libstdc++ at all, so you have to
  compile with g++ instead (hence the dependency)

* On OSX, you have to link with the `-lclangEdit` library, but that library
  doesn't exist on Fedora

* node-gyp disables C++ exceptions by default (compiles with `-fno-exceptions`),
  so you have to make sure to turn them on by overriding the default in
  binding.gyp

* node doesn't like hyphens in module names in the `NODE\_MODULE` macro, so you
  just have to use the standard `extern "C" void init(...)` function.
  

Dependencies
============
    sudo yum -y install clang clang-devel gcc-c++ llvm llvm-devel
