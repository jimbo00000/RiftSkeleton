Using OSVR
==========

Using the OSVR libraries and server with the Oculus Rift requires some extra setup on top of installing the OVR SDK and runtime. Install the 0.5.0.1 Oculus runtime and SDK and make sure the service is running. Set the Rift display mode to Extended (This is only so we can create a window without access to Oculus's Direct Mode).

Be sure to leave the Rift's screen orientation as Portrait (as it is by default) to reduce judder.

## Setup

Download binaries for the OSVR-Core and Oculus Rift Plugin packages from [osvr.github.io](http://osvr.github.io/using/)

**Known working builds for Windows 8.1:**
[v0.2-338](http://access.osvr.com/binary/download/builds/OSVR-Core/OSVR-Core-Snapshot-v0.2-338-ga848a4c-build122-vs12-32bit.7z) + 
[build78-v0.1-22](http://access.osvr.com/binary/download/builds/OSVR-Oculus-Plugin/OSVR-Oculus-Plugin-build78-v0.1-22-ga9bedc6-with-core-v0.2-338-ga848a4c-32bits.7z)


Place the Oculus Rift Plugin DLL `com_osvr_OculusRift.dll` in `bin/osvr-plugins-0/`.

Copy the json file `osvr_server_config.oculusrift.sample.json` to the directory containing the `osvr_server.exe` executable.

## Starting the Server

Launch `osvr_server.exe` with the json file as the first parameter by dragging the json file onto the exe in Windows explorer. This drag-and-drop operation is the equivalent of running `osvr_server.exe osvr_server_config.oculusrift.sample.json` from the command line.


