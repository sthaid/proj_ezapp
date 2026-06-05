export ANDROID_NDK_ROOT := $(HOME)/android/sdk/ndk/27.0.12077973

TOOLCHAIN_PATH=$(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin

ifeq ($(wildcard $(TOOLCHAIN_PATH)/.),)
    $(error Error: The directory '$(TOOLCHAIN_PATH)' does not exist)
endif

export PATH := $(TOOLCHAIN_PATH):$(PATH)

build: libs/arm64-v8a/libcrypto.so

libs/arm64-v8a/libcrypto.so:
	# clone and build openssl
	( cd /tmp; \
          rm -rf openssl; \
          git clone https://github.com/openssl/openssl; \
          cd openssl; \
          ./Configure android-arm64 shared; \
          make )
	# populate the openssl dir from the openssl build
	rm -rf include libs
	mkdir -p include libs/arm64-v8a
	cp -r /tmp/openssl/include/openssl include
	cp  /tmp/openssl/libcrypto.so libs/arm64-v8a
	cp  /tmp/openssl/libssl.so libs/arm64-v8a
	# cleanup /tmp
	rm -rf /tmp/openssl

clean:
	rm -rf libs include
