# ezApp

EzApp runs miniApps that are written in the C Language.
Various miniApps are included with ezApp.

Users can also develop your own miniApps, or modify the included miniApps.
To modify or develop miniApps a Linux PC is required

When ezApp is first run on your Android device, the following 
permissions will be requested:
- Post Notifications
- Access Coarse Location
- Access Fine Location 
- Activity Recognition   
- Record Audio
If some of these permissions are not granted then some ezApp capabilities
will not function. For example, if 'Activity Recognition' is not granted
the Step counter will not function.

# Included miniApps

The following miniApps are included. The C language source code for these 
miniApps is included in ezApp. Each miniApp contains a README which can be
viewed by selecting '?'.

- Altitude: Displays current altitude, graphs history..
- Calc:     Hex / Decimal calculator, 32 or 64 bit selectable.
- Clock:    Analog clock, also displays sunrise and sunset times.
- ColrOrgn: Record or play audio, and display color organ.
- Compass:  Displays magnetic or true heading.
- FlshLite: Toggles device flashlight on/off.
- Light:    Simple example, sets entire screen to red or white.
- Location: Displays current location and location history.
- Log:      Displays developer message log.
- Memo:     Record an audio memo.
- Morse:    Practice morse code, rate selectable between 5 and 20 WPM.
- Paddle:   Ball and paddle game.
- Piano:    Beginner Piano simulator. Includes several public domain melodies.
- Reversi:  Play Reversi against the computer.
- Steps:    Displays Steps and Miles for specified day, month, or year.
- Template: Simple 'Hello World' example.
- Test:     Unit Test.
- Tilt:     Level, supports horizontal and vertical orientations, and calibration.
- Weather   Displays and speaks the weather forecast, from weather.gov.

apps

services

# Developing miniApps

Summary of how ezApp works:
- The Simple DirectMedia Layer (SDL) provides the framework from which the
  Android ezApp is built. SDL provides a Java Shim, which calls the C language
  ezApp entry point, called SDL_main, and found in src/ezapp/main.c.
- SDL provides functions for: Graphics & Rendering, Input Handling, Audio, Events, and more.
- The PicoC C language interpreter is used to execute the miniApps. PicoC includes support
  for many standard C language header files, such as stdio.h, string.h, etc.
- Additional header files are added to PicoC to support ezApp. Support for these 
  additional header files is added to picoc/platform/library_unix.c. These header files are:
  - sdlx.h:  provides a subset of SDL.h, including support for core SDL functions that
    are required by miniApps.
  - utils.h: miscellaneous utilities, includes support for json, png, fft, and access
    to Android java code for location, text-to-speach, and playback capture.
  - svcs.h:  miniApps sometimes rely on miniSvcs; for example the Location miniApp
    relies on Location info provided by the Location miniSvc. A miniApp can call 
    svc_make_req() to make a request to a miniSvc. MiniSvcs also can share data
    with miniApps using a shared data file.
- In addition to being embedded in picoc/platform/library_unix, the sdlx.h, utils.h, and svcs.h
  files are also available in src/ez_lib/include. These files should be viewed there
  for documentation of their capabilities.

git clone
dir struct

Steps to add a new miniApp:
- 





# developing ezApp








  files.kl
    that klllllllllllllllllllllll
https://github.com/jpoirier/picoc

Some of the following description will provide commands that are specific to Ubuntu,
such as 'apt install'. If you are using a differnet Linux distro, or an old Linux
version, you may need to make adjustments.

- ubuntu 2510
  build-essential

- clone
- build    separate build for this, compared to android
                OR build android last
- update path

- on device
  - enable devel mode
  - password

- using ezsh
- using eztest
- using ezput

- dir struct notes
- note on files/apps/layout
          files/svcs/svcs


cd files/apps/Templste
vi template.c
eztest   
ezput

ezsh samsung secret log

run the app on Android


mkdir files/apps/New
cd files/apps/New
vi main.c
vi ../layout
ezput

# Developing ezApp

Android/Sdk

refer to another Readme
- running on Linux


----------------------------------------------
----------------------------------------------
----------------------------------------------

# Table of Contents
- [Installation](#installation)
- [Configuration Options](#configuration-options)
- [Frequently Asked Questions](#frequently-asked-questions)
  - [Second Level](#second-level)


# Installation
Content goes here...

# Configuration Options
Content goes here...

# Frequently Asked Questions
Content goes here...

## second level
Content goes here...

----------------------------------------------

Headings

# Hello
## Hello
### Hello

----------------------------------------------

Lists

- George Washington
- George Washington
- George Washington

1. James Madison
2. James Monroe
3. John Quincy Adams

----------------------------------------------

text

start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.  start a sentence.

start a sentence.  start a sentence.  start a sentence.  start a sentence.  

using backslash to end a line\
using backslash to end a line\
using backslash to end a line\
all done

more stuff

----------------------------------------------

Quotes

> text is a quote

> text is a quote\
> text is a quote\
>
> text is a quote
> text is a quote

Use `git status` to list all new or modified files that haven't yet been committed.

Some basic Git commands are:
```
git status
git add
git commit
```

----------------------------------------------

Links

this site was built using [GitHub Pages](https://pages.github.com/).

----------------------------------------------

Code blocks

    int main() {
        printf("hello world\n");
    }

----------------------------------------------

Github ...

> [!NOTE]
> Useful information that users should know, even when skimming content.

> [!TIP]
> Helpful advice for doing things better or more easily.

> [!IMPORTANT]
> Key information users need to know to achieve their goal.

> [!WARNING]
> Urgent info that needs immediate user attention to avoid problems.

> [!CAUTION]
> Advises about risks or negative outcomes of certain actions.


