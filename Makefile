
SUBDIRS = \
    linux \
    android \
    src/picoc \
    bin/src 

APPS = \
    files/apps/Clock \
    files/apps/Compass \
    files/apps/FlshLite \
    files/apps/HexCalc \
    files/apps/Light \
    files/apps/Location \
    files/apps/Morse \
    files/apps/Paddle \
    files/apps/Reversi \
    files/apps/Template \
    files/apps/Test \
    files/apps/Tilt \
    files/apps/Weather \
    files/apps/Memo

SVCS = \
    files/svcs/Location \
    files/svcs/Template

build:
	for d in $(SUBDIRS) ; do echo "\n======== BUILD $$d ========\n"; make -C $$d || exit 1; done
	for d in $(APPS)    ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done
	for d in $(SVCS)    ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

apps:
	for d in $(APPS)    ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

svcs:
	for d in $(SVCS)    ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

clean:
	for d in $(SUBDIRS) ; do echo "\n======== CLEAN $$d ========\n"; make -C $$d clean || exit 1; done
	rm -f cscope.out cscope.files tags

.PHONY: build clean
