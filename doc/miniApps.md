Create a miniApp xxx UNDER CONSTRUCTION 
=======================================

xxx description

Install ezApp on your Android device
====================================

xxx instructions - load from Google Play

can stop here and just use the provided miniApps

xxx enable ezApp developer mode

env vars to use ezsh, mention options if you dont want to do this
export EZAPP_DEVICE=<dev-ip-addr>
export EZAPP_PASSWD=<devel-mode-password>

xxx you do not need to enable Android devel mode

Setup Development PC
====================

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

xxx
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
- refer to ezApp/src/picoc/README.md, especially the "How PicoC differs from C90" section.

When the PicoC interpreter encounters code that it doesn't understand the error location
is identifiec, for example:
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

The primary limitations of PicoC are outlined below.

Macros must not be defined within a procedure, and macros must return a value.
Examples of good macros:
```C
#define ACOSD(x)  (acos(x)*RAD2DEG)
#define DEG2RAD   (M_PI / 180.)
```

The goto statement is implemented but only supports forward gotos, not backward.

Short-circuit evaluation is not supported. For example the following code
will print "x=0" in standard C, and print "x=1" in PicoC.
```C
    int x = 0;
    if (true || x++) {
        printf("x=%d\n", x);
    }
```

Pointers to procedures are not supported.

Stdarg is not supported, '#include <stdarg.h>' will fail.

Floating point numbers must not start with '.'. For example ```x = .123;``` is not supported.
Instead use ```x = 0.123```.

Nested ternary operator may give incorrect result. For exmple: ```(true ? 1 : true ? 2 : 3);``` evaluates 
to 2 in PicoC, it should evaluate to 1. Instead use ```(true ? 1 : (true ? 2 : 3))```.

Static array declarations must include the number of array elements. For example:
```static int x[] = {1,2,3};``` fails. Instead use ```static int x[3] = {1,2,3};```

Pointer arithmetic issue:
```
int x[] = {1,2,3};
printf("%p\n", x+1);    // this fails to execute in PicoC
printf("%p\n", &x[1]);  // use this instead

```

Another pointer arithmetic issue:
```
int x[] = {1,2,3};
printf("%ld\n", &x[1] - &x[0]);    // prints '1' in standard C
printf("%ld\n", &x[1] - &x[0]);    // prints '4' in PicoC

```

Initializing an array of struct is not supported. This fails in PicoC:
```
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

