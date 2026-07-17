UNDER CONSTRUCTION
==================

ezApp
=====

EzApp runs miniApps and miniSvcs that are written in the C Language.
These miniApps and miniSvcs are executed by the PicoC C language interpreter.

xxx explain intended to run on Android app; 
xxx also runs on Linux PC for testing; without Android sensors

Users can develop their own miniApps and miniSvcs.
To develop miniApps and miniSvcs a PC is required. Refer to xxx for instructions.

When ezApp is first run on your Android device, the following permissions
will be requested:

- Post Notifications
- Access Coarse Location
- Access Fine Location 
- Activity Recognition   
- Record Audio

If some of these permissions are not granted then some ezApp capabilities
will not function. For example, if 'Activity Recognition' is not granted
the Step counter will not function.

Source code is here xxx: https://github.com/sthaid/proj_ezApp.git.
See the doc directory for details.

miniApps
========

The following miniApps are included, along with their C language source code.
Each miniApp contains a README which can be viewed by tapping the '?'.

- Altitude: View current altitude, and history.
- Calc:     Hex / Decimal calculator, 32 or 64 bit selectable.
- Clock:    Analog clock, also displays sunrise and sunset times.
- ColrOrgn: Record or play audio, and display color organ.
- Compass:  View magnetic or true heading.
- FlshLite: Toggles device flashlight on/off.
- Light:    Sets entire screen to red or white.
- Location: View current location and location history.
- Log:      View  message from ezApp and miniApp printf.
- Memo:     Record an audio memo.
- Morse:    Practice morse code, rate selectable from 5 to 20 WPM.
- Paddle:   Ball and paddle game.
- Piano:    Beginner Piano simulator. Includes several public domain melodies.
- Reversi:  Play Reversi against the computer.
- Steps:    View Steps and Miles for specified day, month, or year.
- Template: 'Hello World' example.
- Test:     Unit Test.
- Tilt:     Level, supports horizontal and vertical orientations, and calibration.
- Weather   View and speaks the weather forecast, from weather.gov.

miniSvcs
========

MiniSvcs run in the background while ezApp is active. The MiniSvcs continue
to run when ezApp is backgrounded, or the Android is in Doze Mode. 

The following MiniSvcs are included. These provide support for miniApps
by saving data in files, or responding to the svc_make_req call issued
by a miniApp.

- Altitude: Saves history of altitude.
- Location: Saves history of city/town location.
- Steps:    Saves history of step counts.
- Template: A miniSvc example.

MiniSvcs can be stopped or started by selecting "Settings", "Services".

