//
// Copyright (c) 2026 Nightwind
//

#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <dlfcn.h>

void GetImageByNameTests(void) {
    const void *const crashReporterHandle = dlopen("/System/Library/PrivateFrameworks/CrashReporterSupport.framework/CrashReporterSupport", RTLD_NOW);
    assert(crashReporterHandle != NULL);

    // Find existing image
    {
        LoadedImageReference image = GetImageByName("/System/Library/PrivateFrameworks/CrashReporterSupport.framework/CrashReporterSupport");
        assert(image != NULL);

        Dl_info info;
        int dladdrStatus = dladdr(image, &info);
        assert(dladdrStatus != 0);

        assert(strcmp(info.dli_fname, "/System/Library/PrivateFrameworks/CrashReporterSupport.framework/CrashReporterSupport") == 0);
    }

    // Find already loaded image
    {
        LoadedImageReference image = GetImageByName("/System/Library/Frameworks/Foundation.framework/Foundation");
        assert(image != NULL);

        Dl_info info;
        int dladdrStatus = dladdr(image, &info);
        assert(dladdrStatus != 0);

        assert(strcmp(info.dli_fname, "/System/Library/Frameworks/Foundation.framework/Foundation") == 0);
    }

    // Find unloaded image
    {
        LoadedImageReference image = GetImageByName("/System/Library/PrivateFrameworks/Bom.framework/Bom");
        assert(image == NULL);
    }

    // Precondition failure
    {
        _Pragma("GCC diagnostic push");
        _Pragma("GCC diagnostic ignored \"-Wnonnull\"");
        
        {
            LoadedImageReference image = GetImageByName(nil);
            assert(image == NULL);
        }
        
        _Pragma("GCC diagnostic pop");
    }
}
