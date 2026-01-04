CMAKE_INSTALL_PREFIX = $(PWD)/local
CMAKE_PREFIX_PATH    = $(PWD)/local

ezapp:
	../bin/build_tools/gen_version
	mkdir -p build/$@
	cd build/$@; cmake ../../../src/$@ -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
	cd build/$@; cmake --build . # --verbose

SDL SDL_ttf SDL_mixer picoc lodepng cJSON mp3lame ezapp_lib:
	mkdir -p build/$@
	cd build/$@; cmake  ../../../src/$@ -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cd build/$@; cmake --build . # --verbose
	cd build/$@; cmake --install .

all: SDL SDL_ttf SDL_mixer picoc lodepng cJSON mp3lame ezapp_lib ezapp

mini: picoc lodepng cJSON mp3lame ezapp_lib ezapp

run: ezapp
	cd ../files; ../linux/build/ezapp/ezapp

clobber:
	rm -rf build local

.PHONY: SDL SDL_ttf SDL_mixer picoc lodepng cJSON mp3lame ezapp_lib 
.PHONY: ezapp run all mini clean

