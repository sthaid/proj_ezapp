# --- build ---

APPS := $(sort $(dir $(wildcard files/apps/*/.)))
APPS := $(filter-out files/apps/lib/, $(APPS))
SVCS := $(sort $(dir $(wildcard files/svcs/*/.)))

build: clone_sdl bin/src linux test_build_apps_and_svcs

clone_sdl:
	if [ ! -d src/SDL -o ! -d src/SDL_ttf -o ! -d src/SDL_mixer ]; then bin/build_tools/clone_sdl; fi

bin/src linux:
	make -C $@

test_build_apps_and_svcs:
	for d in $(APPS) ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done
	for d in $(SVCS) ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

.PHONY: build clone_sdl bin/src linux test_build_apps_and_svcs 

# --- android build & install ---

build_android: 
	make -C src/openssl
	make -C android

.PHONY: build_android

# --- clobber ---

clobber:
	use git clean -fdx

.PHONY: clobber
