BRLCAD Module for OSPRay
========================

Build instructions:

1. Clone this repository into [ospray/source/location]/modules
2. Checkout 'devel' branch of OSPRay: git checkout devel
3. Set BRLCAD_ROOT to where BRLCAD is installed on your machine
4. Enable in OSPRay build with OSPRAY_MODULE_BRLCAD=ON CMake option

Run ospBrlcadViewer with:

./ospBrlcadViewer -g [path/to/.g/file] -o [comma,separated,list,of,objects]

NOTE: At the moment, having a large number of threads running causes BRLCAD to crash, thus it is recommended to use '--osp:numthreads [num_threads]' command line option to reduce the number of threads until BRLCAD is more robust.

