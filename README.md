RiftSkeleton
============

A basic OpenGL app for the Oculus VR SDK.

## Description 
The skeleton app targets the OVR SDK v0.4.1 and provides a simple framework for developing OpenGL VR programs. Take a look at the Scene class under src/ for a basic "hello world" implementation(some simple color cubes in space).

## Features 
 - OVR SDK and Client rendering paths  
 - Adaptive render buffer resolution scaling to ensure fastest possible frame rate(Client path only)  
 - Camera frustum highlighting when headset approaches limits of tracking area  
 - Auxiliary window with AntTweakbar controls(toggle with backtick(`) press)  
 - Tap HMD to hide Health and Safety warning  
 - Sixense SDK Hydra support  
 - Keyboard and gamepad world motion support
 - Interchangeable GLFW and SDL2 backends  

## Portability 
The app is designed to be portable across all systems targeted by the OVR SDK(Windows, Mac, Linux). Unfortunately, Linux is not yet supported by the recent versions(0.4.x) of the OVR SDK.

