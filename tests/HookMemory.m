#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <dlfcn.h>

void HookMemoryTests(void) {
    NSBundle *const bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/Preferences.framework"];
    assert(bundle != NULL);
    
    assert([bundle load]);

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
        pid_t (*orig_getpid)(void) = (__typeof__(orig_getpid))dlsym(RTLD_DEFAULT, "getpid");
        assert(orig_getpid != NULL); 

        const pid_t realPid = getpid();

        const uint8_t patch[] = {
            0x39, 0x00, 0xA0, 0xE3, // mov r0, #57
            0x1E, 0xFF, 0x2F, 0xE1 // bx lr
        };

        MicroInjectorReturn_t hookStatus = HookMemory((void *)orig_getpid, patch, sizeof(patch));
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        assert(getpid() == 57);
        assert(getpid() != realPid);
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
