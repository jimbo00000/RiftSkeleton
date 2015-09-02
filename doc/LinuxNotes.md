Running RiftSkeleton on Linux
============

## Set up the Rift Display

Turn on the Rift (blue light should be on)
Run `nvidia-settings` as root
Click the **X Server Display Configuration** tab
Select the Rift DK2 screen (portrait orientation)
Under the Configuration drop menu, choose "New X Screen (requires X Restart)"
Click the "Apply what is Possible" button (althought it probably does nothing)
Click the "Save to X Configuration File" button
Restart X or just reboot (I can'd find a clear way to restart X on Fedora 21)

## Run ovrd
Assuming the 0.5.0.1 SDK is built and installed, run `sudo /usr/local/bin/ovrd`

## Run the app
Run cmake as usual, cd build && make, then `DISPLAY=:0.1 sudo ./RiftSkeleton`
