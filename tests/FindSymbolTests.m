//
// Copyright (c) 2026 Nightwind
//

#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

static MI_NULLABLE LoadedImageReference test_image_for_path(const char *MI_NONNULL path) {
    const uint32_t count = _dyld_image_count();
    
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(_dyld_get_image_name(i), path) == 0) {
            return _dyld_get_image_header(i);
        }
    }
    
    return NULL;
}

void FindSymbolTests(void) {
    void *crashReporterHandle = dlopen("/System/Library/PrivateFrameworks/CrashReporterSupport.framework/CrashReporterSupport", RTLD_NOW);
    assert(crashReporterHandle != NULL);
    
    LoadedImageReference const crashReporterImage = test_image_for_path("/System/Library/PrivateFrameworks/CrashReporterSupport.framework/CrashReporterSupport");
    assert(crashReporterImage != NULL);
    
    LoadedImageReference const foundationImage = test_image_for_path("/System/Library/Frameworks/Foundation.framework/Foundation");
    assert(foundationImage != NULL);

    // Find exported symbol with handle
    {
        void *symbol = FindSymbol(crashReporterImage, "_CRSubmitProblemReport");
        assert(symbol != NULL);

        void *dlsymSymbol = dlsym(crashReporterHandle, "CRSubmitProblemReport");
        assert(symbol == dlsymSymbol);
    }

    // Find exported symbol with no handle
    {
        void *symbol = FindSymbol(NULL, "_CRGenerateReportWithCodeAndDescription");
        assert(symbol != NULL);

        Dl_info info;
        int dladdrStatus = dladdr(symbol, &info);
        assert(dladdrStatus != 0);

        assert(strcmp(info.dli_fname, "/System/Library/PrivateFrameworks/CrashReporterSupport.framework/CrashReporterSupport") == 0);
        assert(strcmp(info.dli_sname, "CRGenerateReportWithCodeAndDescription") == 0);
    }

    // Find non-exported symbol with handle
    {
        void *symbol = FindSymbol(foundationImage, "___NSSetCStringCharToUnichar");
        assert(symbol != NULL);

        Dl_info info;
        int dladdrStatus = dladdr(symbol, &info);
        assert(dladdrStatus != 0);

        assert(strcmp(info.dli_fname, "/System/Library/Frameworks/Foundation.framework/Foundation") == 0);
        assert(strcmp(info.dli_sname, "__NSSetCStringCharToUnichar") != 0);
    }

    // Find non-exported symbol with no handle
    {
        void *symbol = FindSymbol(NULL, "__NSStandardizePathUsingCache");
        assert(symbol != NULL);

        Dl_info info;
        int dladdrStatus = dladdr(symbol, &info);
        assert(dladdrStatus != 0);

        assert(strcmp(info.dli_fname, "/System/Library/Frameworks/Foundation.framework/Foundation") == 0);
        assert(strcmp(info.dli_sname, "_NSStandardizePathUsingCache") != 0);
    }

    // Symbol not found in specified image
    {
        void *symbol = FindSymbol(crashReporterImage, "_NSLog");
        assert(symbol == NULL);
    }

    // Symbol not found in any image
    {
        void *symbol = FindSymbol(NULL, "_NonexistentSymbol");
        assert(symbol == NULL);
    }

    // Symbol correct in one image but not another
    {
        void *symbolInFoundation = FindSymbol(foundationImage, "_NSLog");
        assert(symbolInFoundation != NULL);

        void *symbolInCrashReporter = FindSymbol(crashReporterImage, "_NSLog");
        assert(symbolInCrashReporter == NULL);
    }

    // Precondition failure
    {
        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wnonnull\"")
        void *symbol = FindSymbol(NULL, NULL);
        assert(symbol == NULL);
        _Pragma("GCC diagnostic pop")
    }
}
