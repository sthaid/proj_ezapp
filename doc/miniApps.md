# Create a miniApp xxx UNDER CONSTRUCTION 

xxx description

# Install ezApp on your Android device

xxx instructions - load from Google Play

can stop here and just use the provided miniApps

xxx enable ezApp developer mode

env vars to use ezsh, mention options if you dont want to do this
export EZAPP_DEVICE=<dev-ip-addr>
export EZAPP_PASSWD=<devel-mode-password>

xxx you do not need to enable Android devel mode

# Setup Development PC

requires devel PC
The instructions provided by this document have been tested on:
- Ubuntu 25.10   xxx
- Ubuntu 26.04   xxx
- Wsl            xxx

Install packages:
```
sudo apt update
sudo apt install -y git
sudo apt install -y build-essential
sudo apt install -y openjdk-17-jdk-headless
sudo apt install -y ssl-dev
sudo apt install -y cscope universal-ctags
```

Clone ezApp ```cd ~; git clone https://github.com/sthaid/ezApp.git```

Set PATH environment variable  ```export PATH=$PATH:~/ezApp/bin```

Build ezApp: ```cd ~/ezApp; make```
This will take several minutes, and performs the following steps:
- clone_sdl: git clone the SDL repos
- bin/src: build tools in the bin dir
- linux: build a version of ezApp that runs on the devel PC
- test_build_apps_and_svcs: performs test build of all provided miniApps and miniSvcs

Perform tests to validate the setup was successful.
- cd ~/ezApp/linux; make run
- ezsh ls apps

# Create a new miniApp

Create a new miniApp, starting with a copy of the Template miniApp.

```
cd ~/ezApp/files/apps
cp -r Template NewApp
cd NewApp
vi template.c    # change "Hello\nWorld" to "NewApp"
```

# Test the new miniApp on Development PC

xxx
Why do this ..

Perform test build the miniApp on the Linux devel PC; and run it using picoc
This step is recomended, but not required.
eztest runp

Perform Test cd linux make run, also optional

# Run the new miniApp on Android

Start ezApp on your Android device. Ensure developer mode is enabled and 
password is set. xxx reword.

In a new terminal session run ```ezsh logwatch``` to view ezApp log messages.

Copy the the new miniApp to the Android Device.
```cd ~/ezApp/files/apps/NewApp; ezput```

On Android ezApp menu, locate and tap NewApp.

# APIs available for use by miniApps

Picoc is extended to support the APIs defined in src/exApp_lib/include/...

These APIs are availabe for use by miniApps

Refer to src/exApp_lib/include/ ...
- sdlx.h
- utils.h
- svcs.h

How these are incorporated in picoc

cstldlib in picoc

give an example of cstdlib file

# PicoC Limitations

# bin dir 

xxx check that these all have -h 

These are the main bin xxx used to develop miniApps

eztest: performs a test run of a miniApp or miniSvc on the Devel PC
ezput:  copies miniApps and miniSvcs to Android
ezsh:   establishes a network connection to ezApp on Android, and simulates a shell running on the Android device

Refer to bin/README.md for more.

