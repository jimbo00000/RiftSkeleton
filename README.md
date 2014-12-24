RiftSkeleton
============

A basic OpenGL app for the Oculus VR SDK.

## Description 
The skeleton app targets the OVR SDK v0.4.4 and provides a simple framework for developing OpenGL VR programs. Take a look at the Scene class under src/ for a basic "hello world" implementation(some simple color cubes in space).

## Portability 

 - Linux, MacOS, Windows  
 - NVIDIA, AMD

## Dependencies 
 - [CMake](http://www.cmake.org/) (for building)
 - [GLFW](http://www.glfw.org/download.html) or [SDL2](https://www.libsdl.org/download-2.0.php)
 - [GLEW](http://glew.sourceforge.net/)
 - [GLM](http://glm.g-truc.net/0.9.6/index.html)
 - [Oculus SDK](https://developer.oculus.com/downloads/) (optional)
 - [Sixense SDK](http://sixense.com/windowssdkdownload) (optional)
 - [AntTweakbar](http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:download) (optional)

I set up my local build environment with libraries installed under a single directory(**C:/lib** on Windows, **~/lib** on Linux, **~/Development** on MacOS). This location can be changed in cmake-gui by modifying the **LIBS_HOME** variable or by editing it in CMakeLists.txt directly.

## Features 
 - OVR SDK and Client rendering paths  
 - Adaptive render buffer resolution scaling to ensure fastest possible frame rate(Client path only)  
 - Camera frustum highlighting when headset approaches limits of tracking area  
 - Auxiliary window with AntTweakbar controls(toggle with backtick(`) press)  
 - Tap HMD to hide Health and Safety warning  
 - Sixense SDK Hydra support  
 - Keyboard and gamepad world motion support
 - Interchangeable GLFW and SDL2 backends  
