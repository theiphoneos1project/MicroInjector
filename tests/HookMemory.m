#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <dlfcn.h>

void HookMemoryTests(void) {
    NSBundle *const preferencesBundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/Preferences.framework"];
    assert(preferencesBundle != NULL);
    assert([preferencesBundle load]);
    
    NSBundle *const crashReporterBundle = [NSBundle bundleWithPath:@"/System/Library/PrivateFrameworks/CrashReporterSupport.framework"];
    assert(crashReporterBundle != NULL);
    assert([crashReporterBundle load]);

    // Hook global variable
    {
        void **key = dlsym(RTLD_DEFAULT, "PSDetailControllerClassKey");
        assert(key != NULL);

        assert([(__bridge NSString *)*key isEqualToString:@"detail"]);

        CFStringRef replacement = CFSTR("hooked");
        MicroInjectorReturn_t hookStatus = HookMemory(key, &replacement, sizeof(replacement));
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        assert([(__bridge NSString *)*key isEqualToString:@"hooked"]);
    }

    // Patching executable code
    {   
        BOOL (*CRSubmitProblemReport)(void) = (__typeof__(CRSubmitProblemReport))dlsym(RTLD_DEFAULT, "CRSubmitProblemReport");
        assert(CRSubmitProblemReport != NULL);
        assert(CRSubmitProblemReport() == NO);
        
        const uint8_t patch[] = {
            0x01, 0x00, 0xA0, 0xE3, // mov r0, #1
            0x1E, 0xFF, 0x2F, 0xE1  // bx lr
        };
        
        MicroInjectorReturn_t hookStatus = HookMemory((void *)CRSubmitProblemReport, patch, sizeof(patch));
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        assert(CRSubmitProblemReport() == YES);
    }

    // Precondition failures
    {
        _Pragma("GCC diagnostic push");
        _Pragma("GCC diagnostic ignored \"-Wnonnull\"");

        void **key = dlsym(RTLD_DEFAULT, "PSDetailControllerClassKey");
        assert(key != NULL);

        CFStringRef replacement = CFSTR("hooked");
        
        {
            MicroInjectorReturn_t hookStatus = HookMemory(nil, &replacement, sizeof(replacement));
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }
        
        {
            MicroInjectorReturn_t hookStatus = HookMemory(key, nil, sizeof(replacement));
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }
        
        {
            MicroInjectorReturn_t hookStatus = HookMemory(key, &replacement, 0);
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }
        
        _Pragma("GCC diagnostic pop");
    }
}
