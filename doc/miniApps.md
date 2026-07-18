Create a miniApp
================

This document describes how to:
- install and setup ezApp on your Android Smartphone
- create a new miniApp using a Development PC
- transfer the new miniApp from the Development PC to the ezApp
- run the new miniApp on your Android Smartphone

Install ezApp on your Android device
====================================

Install ezApp from the Google Play Store.

Open ezApp, accept the permission requests, and try out the miniApps that
are included with ezApp. Select Settings > Readme for a brief description of the
miniApps that are included.

If you want to create a new miniApp then continue with the steps described in this document.

Enable ezApp Developer Mode.
- Open ezApp, 'Settings' selection should be at bottom right of display
- Settings > Devel_Mode > Tap to set ON
- Settings > Devel_Password > Tap to choose a password

You do not need to enable Android Device Developer Options to create miniApps.

Setup Development PC
====================

A PC running Windows 11 or Ubuntu 25.10 (or newer) is required.
Other Linux distros may also work.

xxx describe steps for Windows 11 wsl

Install Ubuntu Linux packages:
```
sudo apt update
sudo apt install -y git
sudo apt install -y build-essential
sudo apt install -y openjdk-17-jdk-headless
sudo apt install -y ssl-dev
sudo apt install -y cscope universal-ctags
```

Clone ezApp from github:
```
cd ~
git clone https://github.com/sthaid/ezApp.git
```

Set environment variable
- The Android device should be connected to a trusted Wi-Fi network
- The Android Device IP address is displayed in ezApp > Settings
- The android-device-ip-addr can be an IP address, such as 192.168.1.101 or a Hostname (if available)
- The [:port] is only needed if the ezApp Devel_Port had been changed to something other than the default 9000
```
    export PATH=$PATH:~/ezApp/bin
    export EZAPP_DEVICE=<android-device-ip-addr[:port]>   # example: 192.168.1.101
    export EZAPP_PASSWD=<devel-mode-password>             # example: my-secret-password
```

Build ezApp: This will take several minutes, and performs the following steps:
- git clone the SDL repos
- build tools in the bin/src dir
- build a version of ezApp that runs on the devel PC
- perform a test build of all included miniApps and miniSvcs
```
    cd ~/ezApp
    make
```

Tests to validate.

* Run a Linux build of ezApp on the Devel PC.
```
    cd ~/ezApp/linux
    make run
    terminate with ctrl-c
```

* Verify ezsh, running on the Devel PC, can connect to the Android ezApp, and execute the 'ls' cmd.
This should list the contents of the /data/data/org.sthaid.ezApp/files/apps directory on the 
Android device.
```
    ezsh ls apps
```

Create a new miniApp
====================

Create a new miniApp, starting with a copy of the Template miniApp.

```
    cd ~/ezApp/files/apps
    cp -r Template NewApp
    cd NewApp
    vi template.c    # change "Hello\nWorld" to "NewApp"
```

Test the new miniApp on the Devel PC
====================================

xxx left off here xxx
Why do this ..

Perform test build the miniApp on the Linux devel PC; and run it using picoc
This step is recomended, but not required.
eztest runp

Perform Test cd linux make run, also optional

Run the new miniApp on Android
==============================

Start ezApp on your Android device. Ensure developer mode is enabled and 
password is set. xxx reword.

In a new terminal session run ```ezsh logwatch``` to view ezApp debug print messages.

Copy the the new miniApp to the Android Device.
```cd ~/ezApp/files/apps/NewApp; ezput```

On Android ezApp menu, locate and tap NewApp.  xxx 2nd page

APIs available for use by miniApps
==================================

Picoc is extended to support the APIs defined in src/exApp_lib/include.
Refer to these files for documentation of the APIs.
- sdlx.h
- utils.h
- svcs.h

How these are incorporated in picoc  xxx

cstldlib in picoc  xxx

give an example of cstdlib file xxx

PicoC Limitations
=================

PicoC is not intended to be a complete implementation of ISO C:
- PicoC supports the essential aspects of the C language.
- For more info, refer to ezApp/src/picoc/README.md, especially the "How PicoC differs from C90" section.

When the PicoC interpreter encounters code that it doesn't understand the error location
is identified, for example:
```
    $ cat t1.c
    int main() {
        return "hello";
    }

    $ ./picoc t1.c
        return "hello";
            ^
    t1.c:2:10 can't assign int from char*
```

PicoC limitations are described below.

* Macros must not be defined within a procedure, and macros must return a value.
Examples of supported macros:
```C
    #define RAD2DEG (180. / M_PI)
    #define ACOSD(x)  (acos(x) * RAD2DEG)
```

* The goto statement is implemented but only supports forward gotos, not backward.

* Short-circuit evaluation is not supported. For example the following code
will print "x=0" in standard C, and print "x=1" in PicoC.
```C
    int x = 0;
    if (true || x++) {
        printf("x=%d\n", x);
    }
```

* Pointers to procedures are not supported.

* Stdarg is not supported, '#include <stdarg.h>' will fail.

* Floating point numbers must not start with '.'. For example ```x = .123;``` is not supported.
Instead use ```x = 0.123```.

* Nested ternary operator may give incorrect result. For exmple: ```(true ? 1 : true ? 2 : 3);``` evaluates 
to 2 in PicoC, it should evaluate to 1. Instead use ```(true ? 1 : (true ? 2 : 3))```.

* Static array declarations must include the number of array elements. For example:
```static int x[] = {1,2,3};``` fails. Instead use ```static int x[3] = {1,2,3};```

* Pointer arithmetic issue:
```C
    int x[] = {1,2,3};
    printf("%p\n", x+1);    // this fails to execute in PicoC
    printf("%p\n", &x[1]);  // use this instead
```

* Another pointer arithmetic issue:
```C
    int x[] = {1,2,3};
    printf("%ld\n", &x[1] - &x[0]);    // prints '1' in standard C
    printf("%ld\n", &x[1] - &x[0]);    // prints '4' in PicoC
```

* Initializing an array of struct is not supported. This fails in PicoC:
```C
    struct { 
        int x; 
        int y; 
    } array[3] = { {0,0}, {1,1}, {2,2} };
```

bin dir 
=======

xxx check that these all have -h 

These are the main bin xxx used to develop miniApps

eztest: performs a test run of a miniApp or miniSvc on the Devel PC
ezput:  copies miniApps and miniSvcs to Android
ezsh:   establishes a network connection to ezApp on Android, and simulates a shell running on the Android device

Refer to bin/README.md for more.

