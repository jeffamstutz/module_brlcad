BRLCAD Module for OSPRay
========================

Build instructions:

1. Clone this repository into [ospray/source/location]/modules
2. Checkout 'devel' branch of OSPRay: git checkout devel
3. Set BRLCAD_ROOT to where BRLCAD is installed on your machine
4. Enable in OSPRay build with OSPRAY_MODULE_BRLCAD=ON CMake option

Run ospBrlcadViewer with:

./ospBrlcadViewer -g [path/to/.g/file] -o [comma,separated,list,of,objects]

NOTE: At the moment, there seems to be a non-deterministic crash during BRLCAD initialization. Occasionally the viewer will work, but sometimes it will crash. Crashes seem more likely at higher CPU thread counts.

