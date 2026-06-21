# TODO  

xxx search ezapp,  miniApps, miniSvcs
xxx arm64 arch only;  or maybe don't mention this
xxx TOC
xxx spellcheck
xxx reinstall linux PC to check instructions,  
xxx add -h option to bindir tools
xxx Creating a miniSvc ?
xxx section on building android APK

If you expect to have dozens of Markdown files, a common best practice is to keep the root clean by placing your primary README.md at the top level and moving all other supplemental documentation into a dedicated /docs directory

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
- Log:      Displays message logged by ezApp and miniApps.
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

MiniSvcs run in the background while ezApp is active. The MiniSvcs continue to run when
ezApp is backgrounded, or the Android is in Doze Mode. 

The following MiniSvcs are provided, and provide support for miniApps. The miniSvcs provide 
support by saving data in files, or responding to the svc_make_req call issued by a miniApp.

- Altitude: Saves history of altitude.
- Location: Saves history of city/town location.
- Steps:    Saves history of step counts.
- Template: A miniSvc sample.

MiniSvcs can be stopped or started by selecting "Settings" and "Services".

# xxx Refer to ....
README_miniapps.md
README_Android.md      

# XXXXXXXXXXXXXXXXXXXXXX DETAILS XXXXXXXXXXXXXXXXXXXXXXXXXXX

# ezApp Design 

Ezapp code is available on github.
> git clone https://github.com/sthaid/proj_ezApp.git

The Simple DirectMedia Layer (SDL) provides the framework from which the
Android ezApp APK is built. SDL provides an Android Java Shim, which calls the C language
ezApp entry point, SDL_main, found in src/ezApp/main.c.

SDL provides functions for: Graphics & Rendering, Input Handling, Audio, Events, and more.

The PicoC C language interpreter is used to execute the miniApps. PicoC includes support
for some standard C language header files, such as stdio.h, string.h, etc.

Additional header files are added to PicoC to support miniApps. The following
header files are provided in picoc by code in picoc/platform/library_unix.c.
- sdlx.h:  provides core SDL functions that are required by miniApps.
- utils.h: miscellaneous utilities, including: file access, json, png, fft, location, 
  text to speech, and playback capture.
- svcs.h:  provides ability for a miniApp to make a request to a miniSvc.

The sdlx.h, utils.h, and svcs.h files are also provided in src/ezApp_lib/include.
These files should be viewed there for documentation of their capabilities.

Directory structure:
- files/apps: miniApps Altitude, Calc, Clock, ...
- files/svcs: miniSvcs Altitude, Location, Steps, ...
- linux: build & run ezApp on Linux devel PC
- src:        
  - ezApp: C language main entry point, called from the SDL Java shim
  - ezApp_lib: code for the routines defined in sdlx.h, utils.h and svcs.h
  - SDL, SDL_mixer, SDL_ttf: populated with SDL code by top level Makefile
  - openssl: populated and built when android Makefile is run to build the Android APK
  - cJSON, kissfft, libmp3lame, lodepng, picoc: copies of code from git repos
- bin: contains tools used to develop miniApps for your device
- android: the Android Package Kit (APK) is built here; refer to android/README

Licenses: Refer to files/licenses. All of the source code used is licensed under
a permissive license, except libmp3lame which is LGPL license. Libmp3lame license 
requirements are met by linking it as a separate library, There are no changes
made to the libmp3lame source code.

# Setup Linux PC and Android Device to develop miniApps

To create or update miniApps the following setup is required.
These steps are an example for Ubuntu 25.10.

```
# install required packages
sudo apt install build-essential
sudo apt install ssl-dev
sudo apt install git

# git clone proj_ezApp
cd $HOME
git clone https://github.com/sthaid/proj_ezApp.git

# init env variables
# - replace with your Android device IP address or Name
# - password length must be 4 or more chars
export EZAPP_DEVICE=192.168.1.243
export EZAPP_PASSWD=secret
export PATH=$PATH:~/proj_ezApp/bin

# build the devl environment, this will take several minutes
cd ~/proj_ezApp
make
```

Open ezApp on your Android device:
- Select ezApp 'Settings', on botton right of display
- Enable Devel_Mode
- Set Devel_Password

Perform test on devel PC:

```
$ ezsh
connecting to samsung: 192.168.1.243:9000
ezsh files> pwd
/data/data/org.sthaid.ezApp/files/
ezsh files> ls apps
total 67
drwx------ 2 u0_a412 u0_a412 3452 2026-06-20 09:53 Altitude
drwx------ 2 u0_a412 u0_a412 3452 2026-06-20 09:53 Calc
drwx------ 2 u0_a412 u0_a412 3452 2026-06-20 09:53 Clock
drwx------ 3 u0_a412 u0_a412 3452 2026-06-20 09:53 ColrOrgn
drwx------ 2 u0_a412 u0_a412 3452 2026-06-20 09:53 Compass
--- etc. ---
```

The following scripts are provided in the proj_ezApp/bin directory.
Each of these scripts provides help option -h.
- ezsh:   simulates a shell running on the Android device, and
          provides commands to copy files between the devel PC and
          the Android device
- ezput:  copy files from devel PC to Android device
- ezget:  copy files from Android device to devel PC
- eztest: test build and run a miniApp on the devel PC

Note that ezsh is restriced by the Android OS to accessing only ezApp files
and system tools (such as ls, tar, curl). Ezsh is not able to access
files that are part of other Android apps.

# Create a simple miniApp

The following are steps to build, test, and install a simple
miniApp on your Android device.

Create the new miniApp source code.

```
cd ~/proj_ezApp/files/apps
mkdir MyMiniApp
cd MyMiniApp
cp ../Template/template.c my_mini_app.c
cp ../Template/README .
Edit the my_mini_app.c and README files, change "Hello\nWorld" to "MyMiniApp"
```

Perform test build and run of the miniApp on the Linux devel PC.
This step is recomended, but not required.

```
eztest build   # performs test build using Linux build env
eztest runl    # builds and runs the app using Linux build env
eztest runp    # runs the app using the PicoC C language interpreter
```

Copy the miniApp source code to the Android Device.

```
Edit ../layout   # change one of the '-' to 'MyMiniApp'
ezput            # this will copy the my_mini_app.c and README files to Android device
ezput ../layout  # copy the updated layout file to Android device
```

Run the miniApp on Android device. 
- On devel PC Use `logwatch | grep --color=never MyMiniApp` to monitor prints.
- On Android device, select MyMiniAp


# PicoC

PicoC README says:
> PicoC is a tiny C language, not a complete implementation of C90. It doesn't
> aim to implement every single feature of C90 but it does aim to be close enough
> that most programs will run without modification.

PicoC vs C Language differences:
- xxx
- xxx
- xxx

PicoC is copied from `https://github.com/jpoirier/pico` to proj_ezApp/src/picoc.
The proj_ezApp/src/picoc has been modified to add new features and fix bugs.
To view the modifications:
```
cd ~/proj_ezApp/src/picoc
git diff 89f7b53128c196223bbab6a516e03bf0ab85e124 .       # xxx check this later
```

Summary of the modifications:
- xxx
- xxx
- xxx


# Creating ezApp Android APK



xxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxx

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


