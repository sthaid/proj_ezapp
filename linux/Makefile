CMAKE_INSTALL_PREFIX = $(PWD)/local
CMAKE_PREFIX_PATH    = $(PWD)/local

libmp3lame:
	rm -rf build/libmp3lame
	mkdir -p build/libmp3lame
	cd build/libmp3lame; cmake  ../../../src/lame/libmp3lame -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cd build/libmp3lame; cmake --build .
	cd build/libmp3lame; cmake --install .

cJSON:
	rm -rf build/cJSON
	mkdir -p build/cJSON
	cd build/cJSON; cmake -DBUILD_SHARED_LIBS=OFF ../../../src/cJSON -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cd build/cJSON; cmake --build .
	cd build/cJSON; cmake --install .

testprog:
	rm -rf build/testprog
	mkdir -p build/testprog
	cd build/testprog; cmake ../../../src/testprog -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
	cd build/testprog; cmake --build .
	cd build/testprog; cmake --install .

testlib:
	rm -rf build/testlib
	mkdir -p build/testlib
	cd build/testlib; cmake ../../../src/testlib -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cd build/testlib; cmake --build .
	cd build/testlib; cmake --install .

clean:
	rm -rf build local
