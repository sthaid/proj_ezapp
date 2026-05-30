
SUBDIRS := bin/src linux android 
APPS    := $(sort $(dir $(wildcard files/apps/*/.)))
APPS    := $(filter-out files/apps/lib/, $(APPS))
SVCS    := $(sort $(dir $(wildcard files/svcs/*/.)))

# --- build ---

build: clone_sdl subdirs apps svcs

clone_sdl:
	if [ ! -d src/SDL -o ! -d src/SDL_ttf -o ! -d src/SDL_mixer ]; then bin/build_tools/clone_sdl; fi

subdirs:
	for d in $(SUBDIRS) ; do echo "\n======== BUILD $$d ========\n"; make -C $$d || exit 1; done

apps:
	for d in $(APPS) ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

svcs:
	for d in $(SVCS) ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

# --- clean & clobber ---

clean_subdirs:
	for d in $(SUBDIRS) ; do echo "\n======== CLEAN $$d ========\n"; make -C $$d clean || exit 1; done

clean_cscope_and_tage:
	find . -name ".git" -prune -o \( -name cscope.\* -o -name tags \) -exec rm {} \;

clobber: clean_subdirs clean_cscope_and_tage
	@echo "\n======== REMOVING OTHERS  ========\n"
	rm -f src/ezapp/version.h
	git ls-files --other files | xargs rm -f
	rm -rf src/SDL src/SDL_mixer src/SDL_ttf
	make -C src/picoc clean
	@echo "\n======== REMAINING  ========\n"
	@git ls-files --other
	@echo

# --- install on android ---

install_on_android:
	make -C android install

.PHONY: build clone_sdl subdirs apps svcs clean_subdirs clean_cscope_and_tags clobber
