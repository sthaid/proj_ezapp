This README was prepared while using Ubuntu 25.10.

The steps to build and install a developer created version
of ezApp, on your Android device, are described.

xxx more packages?
xxx move some of this to top level readme

===============================
INSTALL LINUX PACKAGES
===============================

xxx reference the setup script

sudo apt update
sudo apt install build-essential
sudo apt install openjdk-17-jdk

===============================
INSTALL ANDROID SDK
===============================

xxx reference the setup script

# if reinstalling the android sdk, first do this:
  # force kill any cached Gradle processes, and clear gradle daemon cache
    pkill -f gradle
    rm -rf ~/.gradle/daemon/
  # remove the existing android sdk
    rm -rf ~/android

# download android sdk linux command line tools from here:
https://developer.android.com/studio#command-line-tools-only

# unzip the cmdlinetools
mkdir -p ~/android/sdk
cd ~/android/sdk
unzip ~/Downloads/commandlinetools-linux-nnnnnnnn_latest.zip

# move the cmdline-tools, to use the required directory structure
mv cmdline-tools latest
mkdir cmdline-tools
mv latest cmdline-tools

# install sdk packages needed to build ezApp to run on Android
sdkmanager "platform-tools" "platforms;android-35" "build-tools;34.0.0" "ndk;27.0.12077973"

# set android sdk environment varialbles
export ANDROID_HOME=$HOME/android/sdk
export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/27.0.12077973
export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools

===============================
CLONE & BUILD PROJ_EZAPP
===============================

# set environment varialbles
export EZAPP=/your/path/to/proj_ezApp
export PATH=$PATH:$EZAPP/bin:$EXAPP/android/bin

# git clone
cd $(dirname $EZAPP)
git clone https://github.com/sthaid/proj_ezApp.git

# build
cd $EZAPP
make build
make build_android

==========================================
CONNECT DEVEL COMPUTER WITH ANDROID DEVICE
==========================================

On Android Device
- Enable developer mode by tapping build number 8 times.
  - Settings -> About Phone -> Software Information -> Build Number
- Go to Developer Options , and enable
  - USB Debugging
  - Stay Awake

Connect using USB Data cable:
- Connect USB Data cable between Computer and Device
  - select 'allow access' on the device.
- Test connection: adb shell

Or, Connect using Wifi:
- Connect USB Data cable between Computer and Device
  - select 'allow access' on the device.
- On computer: adb tcpip 5555
- Disconnect USB Data cable
- On computer: adb connect <ipaddr|hostname>
- Test connection: adb shell

Notes:
- to restart adb server, if needed: adb kill-server; adb start-server
- to list connected devices: adb devices
- to use a static IP address:
  - configure router to assign a static IP address to your Android device
  - may also need to configure your Android device to not used randomized MAC
      Connections -> Wifi -> Gear -> View More -> MAC Address Type -> Pnone MAC

===============================
INSTALL EZAPP ON ANDROID
===============================

cd $EZAPP
make install_android

=============================================
============  ADDITIONAL INFO  ==============
=============================================

===============================
ADB COMMAND EXAMPLES
===============================

adb help

adb logcat --help
adb logcat -c -b all                      # clear Android log
adb logcat -s SDL SDL/APP AndroidRuntime  # view logs from SDL SDL/APP AndroidRuntime components

adb shell                             # run remote shell
adb shell getprop ro.product.cpu.abi  # get Application Binary Interface

adb tcpip 5555             # restart Android Debug Bridge Daemon listening on TCP port 5555
adb connect 192.168.1.205  # connect to device

adb devices  # list connected devices

adb kill-server   # restart adb server
adb start-server

===============================
ANDROID VERSION NUMBERS
===============================

References:
- https://apilevels.com/
- Google: "list of android architecture versions and API levels"

                Android(*)
Codename        Version     API Level   Year    (subset)
--------        -------     ---------   ----
Baklava         16          36          2025
Nougat          7.0         24          2016
Lollipop        5           21          2015

(*) Android Version found here:: Setup -> About phone -> Software information

From Google AI ...

Android SDK Version (minSdkVersion, targetSdkVersion)
- These are attributes used in your app's main build configuration 
  (like a Gradle file in Android Studio) to declare compatibility
  with specific API levels. 
- minSdkVersion: The minimum API level your application can run on.
  Devices running an Android version with an API level lower than your
  minSdkVersion will not be able to install your app from Google Play.
- targetSdkVersion: The API level your app was tested against and is 
  fully compatible with.
- compileSdkVersion: The API level of the Android platform version you
  compile your app against. You should always use the latest stable 
  version for this to access new features and improvements. 

APP_PLATFORM 
- a variable used specifically within the Android NDK (Native Development
  Kit) build system (primarily in the Application.mk file for ndk-build). 
- It declares the Android API level against which your native C/C++ code
  is compiled. For native code, the APP_PLATFORM essentially acts as the
  minimum required API level for your native libraries, similar to how
  minSdkVersion works for the overall application.
- If not specified in Application.mk, ndk-build typically defaults to a
  minimum API level supported by the NDK itself or tries to infer it from
  your minSdkVersion set in the app's manifest/Gradle file. 

NDK Version:
- explicit configuration, in build.gradle:
        android {
            ...
            ndkVersion "23.1.7779620"
        }
- Automatic Selection:
  If you do not specify an ndkVersion, the Android Gradle Plugin (AGP)
  will automatically select a default version that it is known to be
  compatible with

===============================
MISC
===============================

sdkmanager --help  (summary)
  Usage:
    sdkmanager [--uninstall] [<common args>] [--package_file=<file>] [<packages>...]
    sdkmanager --update [<common args>]
    sdkmanager --list [<common args>]
    sdkmanager --list_installed [<common args>]
    sdkmanager --licenses [<common args>]
    sdkmanager --version
  With --install (optional), installs or updates packages.
      By default, the listed packages are installed or (if already installed)
      updated to the latest version.

To list what files are contained in the APK:
  ln -s ./SDL3-3.2.28/build/org.sthaid.ezApp/app/build/outputs/apk/debug/app-debug.apk app-debug.zip
  unzip -l app-debug.zip | grep lib

To specify which ABIS to build:
  ./gradlew assembleDebug -PANDROID_ABIS=arm64-v8a

gradle requires a Java Development Kit (JDK) or Java Runtime Environment 
JRE) of version 17 or higher to run its engine
