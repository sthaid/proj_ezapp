# Overview

The Simple DirectMedia Layer (SDL) provides the framework from which the
Android ezApp APK is built. SDL provides an Android Java Shim, which calls the C language
ezApp entry point, SDL_main, found in src/ezApp/main.c.

SDL provides functions for: Graphics & Rendering, Input Handling, Audio, Events, and more.

The PicoC C language interpreter is used to execute the miniApps. PicoC includes support
for some standard C language header files, such as stdio.h, string.h, etc.

Additional header files are added to PicoC to provide additional APIs for miniApps. The following
header files are provided in picoc by code in picoc/platform/library_unix.c.
- sdlx.h:  provides core SDL functions that are required by miniApps.
- utils.h: miscellaneous utilities, including: file access, json, png, fft, location, 
           text to speech, and playback capture.
- svcs.h:  provides ability for a miniApp to make a request to a miniSvc.

The sdlx.h, utils.h, and svcs.h files are also provided in src/ezApp_lib/include.
These files should be viewed there for documentation of the APIs they provide.

Directory structure:
- files/apps: miniApps Altitude, Calc, Clock, ...
- files/svcs: miniSvcs Altitude, Location, Steps, ...
- linux: build & run ezApp on Linux devel PC
- src:        
  - ezApp: C language main entry point, called from the SDL Java shim
  - ezApp_lib: code for the API routines defined in sdlx.h, utils.h and svcs.h
  - SDL, SDL_mixer, SDL_ttf: populated with SDL code by top level Makefile
  - cJSON, kissfft, libmp3lame, lodepng, picoc: copies of code from git repos
  - openssl: populated and built when android Makefile is run to build the Android APK
- bin: contains tools used to develop miniApps for your device
- android: the Android Package Kit (APK) is built here

Licenses: Refer to files/licenses, and files/credits.
All source code used is licensed under a permissive license, except:
- libmp3lame: LGPL license.
- FreeMonoBold.ttf: GPL-3.0-or-later, with the Font-exception-2.0

# Settings

xxx todo


