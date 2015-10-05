Running RiftSkeleton on Linux
============

## Set up the Rift Display

 - Turn on the Rift (blue light should be on)
 - Run `nvidia-settings` as root
 - Click the **X Server Display Configuration** tab
 - Select the Rift DK2 screen (portrait orientation)
 - Under the Configuration drop menu, choose "New X Screen (requires X Restart)"
 - Click the "Apply what is Possible" button (althought it probably does nothing)
 - Click the "Save to X Configuration File" button
 - Restart X or just reboot (I can't find a clear way to restart X on Fedora 21)

## Run ovrd
Assuming the 0.5.0.1 SDK is built and installed, run `sudo /usr/local/bin/ovrd`

## Run the app built against OVR SDK
`cmake USE_OCULUSSDK=1`  
`cd build && make`  
`DISPLAY=:0.1 sudo ./RiftSkeleton`  


Building OSVR on Ubuntu 15.04
============

## Dependencies

### libfunctionality
`git clone` [https://github.com/OSVR/libfunctionality.git](https://github.com/OSVR/libfunctionality.git)  
Builds, installs out-of-source with default CMake settings.  

### jsoncpp
`git clone` [https://github.com/open-source-parsers/jsoncpp.git](https://github.com/open-source-parsers/jsoncpp.git)  
`JSONCPP_WITH_CMAKE_PACKAGE=1 JSONCPP_LIB_BUILD_SHARED=1`  

### libusb
`sudo apt-get install libusb-1.0-0-dev libboost-all-dev`  

### opencv-2.4.11
Download from [http://opencv.org/downloads.html](http://opencv.org/downloads.html)  
Builds, installs out-of-source with default CMake settings (takes a while)  

### OSVR Oculus Rift Plugin
`git clone` [https://github.com/OSVR/OSVR-Oculus-Rift.git](https://github.com/OSVR/OSVR-Oculus-Rift.git)  
`make`  
`sudo make install`  

`sudo ldconfig` after each `make install`  

### vrpn-feature-oculus-rift  
Build with `VRPN_USE_OVR=1`  

`git clone` [https://github.com/OSVR/OSVR-Oculus-Rift.git](https://github.com/OSVR/OSVR-Oculus-Rift.git)  
`make && sudo make install`  

## Run the app built against OSVR
`cmake USE_OSVR=1`  
`cd build && make`  

`/usr/local/bin/ovrd`  
`sudo osvr_server /usr/local/./osvr_server_config.oculusrift.sample.json`  

`DISPLAY=:0.1 ./RiftSkeleton`  
