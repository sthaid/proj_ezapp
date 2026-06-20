# NOTES 

xxx search ezapp,  miniApps, miniSvcs
xxx Linux PC setup  build-essential, ssl-dev

# ezApp

EzApp runs miniApps and miniSvcs that are written in the C Language.

Users can develop their own miniApps and miniSvcs.
To develop miniApps and miniSvcs a Linux PC is required.

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

xxx arm64 arch only;  or maybe don't mention this

# Included miniApps

The following miniApps are included. The C language source code for these 
miniApps is included in ezApp. Each miniApp contains a README which can be
viewed by selecting '?', usually in the upper right corner.

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
- Morse:    Practice morse code, rate selectable from 5 to 20 WPM.
- Paddle:   Ball and paddle game.
- Piano:    Beginner Piano simulator. Includes several public domain melodies.
- Reversi:  Play Reversi against the computer.
- Steps:    Displays Steps and Miles for specified day, month, or year.
- Template: Simple 'Hello World' example.
- Test:     Unit Test.
- Tilt:     Level, supports horizontal and vertical orientations, and calibration.
- Weather   Displays and speaks the weather forecast, from weather.gov.

MiniSvcs run in the background while ezApp is running. The MiniSvcs continue to run when
ezApp is backgrounded, or the Android is in Doze Mode. The following MiniSvcs are 
provided. They provide support for miniApps. The miniSvcs provide support by saving data
in files, or responding to svc_make_req call issued by a miniApp.

- Altitude: Saves history of altitude.
- Location: Saves history of city/town location.
- Steps:    Saves history of step counts.
- Template: A miniSvc sample.

MiniSvcs can be stopped or started by selecting "Settings" and "Services".

# ezApp Design 

Ezapp code is available in github.
> git clone https://github.com/sthaid/proj_ezApp.git

The Simple DirectMedia Layer (SDL) provides the framework from which the
Android ezApp APK is built. SDL provides a Java Shim, which calls the C language
ezApp entry point, SDL_main, found in src/ezApp/main.c.

SDL provides functions for: Graphics & Rendering, Input Handling, Audio, Events, and more.

The PicoC C language interpreter is used to execute the miniApps. PicoC includes support
for some standard C language header files, such as stdio.h, string.h, etc.

Additional header files are added to PicoC to support the miniApps. The following
header files are provided in picoc by code in picoc/platform/library_unix.c.
- sdlx.h:  provides core SDL functions that are required by miniApps.
- utils.h: miscellaneous utilities, including: file access, json, png, fft, location, 
  text to speech, and playback capture.
- svcs.h:  provides ability for a miniApp to make a request to a miniSvc.

The sdlx.h, utils.h, and svcs.h files are also available in src/ezApp_lib/include.
These files should be viewed there for documentation of their capabilities.

Directory structure:
- files/apps: miniApps Altitude, Calc, Clock, ...
- files/apps/lib: common code for miniApps
- files/svcs: miniSvcs Altitude, Location, Steps, ...
- linux:      build & run version of ezApp that runs on Linux
- src:        
  - ezApp: C language main entry point, called from the SDL Java shim
  - ezApp_lib: code for the routines defined in sdlx.h, utils.h and svcs.h
  - SDL, SDL_mixer, SDL_ttf: populated with SDL code by top level Makefile
  - openssl: populated and built when android Makefile is run
  - cJSON, kissfft, libmp3lame, lodepng, picoc: copies of code from git repos
- bin: contains tools used to develop miniApps for your device
- android: the Android Package Kit (APK) is built here; refer to android/README

Licenses: Refer to files/licenses. All of the source code used is licensed under
a permissive license, except libmp3lame which is LGPL license. Libmp3lame license 
requirements are met by linking it as a separate library, There are no changes
made to the libmp3lame source code.

# Setup Linux PC and Android Device

To create or update miniApps the following setup steps are required:

xxx ...

# Creating a miniApp

cd files/apps
mkdir MyHello
cd MyHello
cp ../Template/templae.c my_hello.c
cp ../Template/README .

Edit the my_hello.c and README files, change "Hello\nWorld" to "My Hello"

eztest build   # performs test build using Linux build env
eztest runl    # builds and runs the app using Linux build env
eztest runp    # runs the app using the PicoC C language interpreter

Edit ../layout    # change one of the '-' to 'MyHello'

ezput apps/MyHello/README apps/MyHello/my_hello.c apps/layout     # xxx simplify
xxx should be ...
ezput my_hello.c README ../layout

Use 'ezsh logwatch' to view prints from your app.

Run the app on your Android device

to view log
ezsh logwatch      # xxx logwatch should clear, both in android/bin and ezsh.alias
  OR
ezsh logwatch | grep --color=never MyHello


kk




# Creating a miniSvc



# PicoC Limitiations



# Creating ezApp Android APK



xxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxx





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


