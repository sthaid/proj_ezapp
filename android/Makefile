build_debug: SDL gen_version gen_assets
	cd SDL/build/org.sthaid.ezApp; \
        ./gradlew assembleDebug

install_debug: build_debug
	cd SDL/build/org.sthaid.ezApp; \
        adb install -r ./app/build/outputs/apk/debug/app-debug.apk

uninstall:
	adb uninstall org.sthaid.ezApp

clean:
	rm -rf SDL assets setup_files/orig

# -----------------------------

SDL:
	./setup

gen_version:
	../bin/build_tools/gen_version

gen_assets:
	# create files.tar
	cd ../files; \
	git ls-files > /tmp/files.txt; \
	tar -cf ../android/assets/files.tar --files-from /tmp/files.txt; \
	rm -f /tmp/files.txt
	# create files.tar.sig
	date > assets/files.tar.sig

# -----------------------------

.PHONY: build_debug install_debug uninstall clean clobber
.PHONY: gen_version gen_assets

