PREFIX := /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
CC := $(PREFIX)/clang
LD := $(HOME)/ld64/linker/ld64-objc1
SDK := $(THEOS)/sdks/iPhoneOS1.1.5.sdk

STRICT_FLAGS := -Wall -Werror -Wpedantic -Wextra -Wshadow -Wformat=2 -Wnull-dereference -Wswitch-enum -Wno-cast-function-type-mismatch -Wno-nullability-extension -Wno-gnu-zero-variadic-macro-arguments

CFLAGS := -arch armv6 -isysroot $(SDK) $(STRICT_FLAGS) -mthumb -miphoneos-version-min=1.0 --ld-path=$(LD)
OBJC_FLAGS := -fno-objc-arc -fobjc-runtime=macosx-fragile

LIBMICROINJECTOR_SRC := MicroInjector.c HookFunction.c SubstrateShims.c
LIBMICROINJECTOR_OUT := libmicroinjector.dylib
LIBMICROINJECTOR_LDFLAGS := -arch armv6 -isysroot $(SDK) -Wl,-install_name,/usr/lib/$(LIBMICROINJECTOR_OUT) -lobjc -dynamiclib

PXL_WORKINGDIR := _temp
PXL_INSTROOT := $(PXL_WORKINGDIR)/instroot

LOADER_SRC := Loader/MicroLoader.m Loader/MISafeModeAlert.m
LOADER_OUT := Loader/MicroLoader.dylib	
LOADER_CFLAGS := -D_FORTIFY_SOURCE=0 -mfpu=none
LOADER_LDFLAGS := -Wl,-install_name,/usr/lib/$(LOADER_OUT) -dynamiclib $(LIBMICROINJECTOR_OUT) $(OBJC_FLAGS) -lobjc -framework UIKit -framework Foundation -framework CoreFoundation

TRAMPOLINE_SRC := launchd_trampoline/launchd_trampoline.c
TRAMPOLINE_OUT := launchd_trampoline/launchd_trampoline.dylib
TRAMPOLINE_LDFLAGS := -dynamiclib $(LIBMICROINJECTOR_OUT) -lobjc -framework UIKit -framework Foundation -framework CoreFoundation

INJECT_SRC := inject_launchd/inject_launchd.c inject_launchd/task_primitives.c inject_launchd/buttons.c
INJECT_OUT := inject_launchd/inject_launchd
INJECT_LDFLAGS := -framework IOKit -framework CoreFoundation

TEST_SRC := $(wildcard tests/*.m)

.PHONY: all clean test

all: $(LIBMICROINJECTOR_OUT) $(LOADER_OUT) $(TRAMPOLINE_OUT) $(INJECT_OUT)

$(LIBMICROINJECTOR_OUT): $(LIBMICROINJECTOR_SRC)
	$(CC) $(LIBMICROINJECTOR_SRC) $(CFLAGS) $(LIBMICROINJECTOR_LDFLAGS) -o $@
	strip -x $@

$(LOADER_OUT): $(LOADER_SRC) $(LIBMICROINJECTOR_OUT)
	$(CC) $(LOADER_SRC) $(CFLAGS) $(LOADER_CFLAGS) $(LOADER_LDFLAGS) -o $@
	strip -x $@

$(TRAMPOLINE_OUT): $(TRAMPOLINE_SRC) $(LIBMICROINJECTOR_OUT)
	$(CC) $(TRAMPOLINE_SRC) $(CFLAGS) $(TRAMPOLINE_LDFLAGS) -o $@
	strip -x $@

$(INJECT_OUT): $(INJECT_SRC)
	$(CC) $(INJECT_SRC) $(CFLAGS) $(INJECT_LDFLAGS) -o $@
	strip -x $@

$(PXL_INSTROOT):
	mkdir -p $(PXL_INSTROOT)/Library/MicroInjector
	mkdir -p $(PXL_INSTROOT)/usr/lib
	mkdir -p $(PXL_INSTROOT)/System/Library/LaunchDaemons

pxl: all $(PXL_INSTROOT)
	@echo "[+] Creating PXL package"

	cp $(LOADER_OUT) $(PXL_INSTROOT)/Library/MicroInjector/MicroLoader.dylib
	cp $(TRAMPOLINE_OUT) $(PXL_INSTROOT)/Library/MicroInjector/launchd_trampoline.dylib
	cp $(INJECT_OUT) $(PXL_INSTROOT)/Library/MicroInjector/inject_launchd
	cp $(LIBMICROINJECTOR_OUT) $(PXL_INSTROOT)/usr/lib/libmicroinjector.dylib
	cp PxlPkg.plist $(PXL_WORKINGDIR)/PxlPkg.plist
	cp LaunchDaemon.plist $(PXL_INSTROOT)/System/Library/LaunchDaemons/com.nightwind.microinjector_inject_launchd.plist
	cd $(PXL_WORKINGDIR) && zip -r ../MicroInjector.pxl .
	rm -rf $(PXL_WORKINGDIR)

	@echo "[+] Done!"

test: $(LIBMICROINJECTOR_OUT)
	$(CC) $(TEST_SRC) $(CFLAGS) $(OBJC_FLAGS) -lobjc -framework Foundation -framework CoreFoundation $(LIBMICROINJECTOR_OUT) -o tests/MicroInjectorTests

clean:
	rm -f $(LIBMICROINJECTOR_OUT) $(LOADER_OUT) $(TRAMPOLINE_OUT) $(INJECT_OUT) MicroInjector.pxl tests/MicroInjectorTests
