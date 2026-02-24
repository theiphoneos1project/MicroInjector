#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <dlfcn.h>

static int (*PSGetAirplaneMode_orig)(void) = NULL;
static int PSGetAirplaneMode_hook(void) {
    NSLog(@"In PSGetAirplaneMode, orig = %d", PSGetAirplaneMode_orig());
    return 3;
}

static BOOL PSSetAirplaneMode_hook_called = NO;
static void PSSetAirplaneMode_hook(int arg1, int arg2) {
    NSLog(@"In PSSetAirplaneMode, arg1: %d, arg2: %d", arg1, arg2);
    assert(arg1 == 1);
    assert(arg2 == 2);
    PSSetAirplaneMode_hook_called = YES;
}

static BOOL Apple80211GetPower_first_hook_called = NO;
static BOOL (*Apple80211GetPower_first_orig)(int, BOOL *) = NULL;
static BOOL Apple80211GetPower_first_hook(int arg1, BOOL *arg2) {
    Apple80211GetPower_first_hook_called = YES;
    return 1;
}

static BOOL Apple80211GetPower_second_hook_called = NO;
static BOOL (*Apple80211GetPower_second_orig)(int, BOOL *) = NULL;
static BOOL Apple80211GetPower_second_hook(int arg1, BOOL *arg2) {
    Apple80211GetPower_second_hook_called = YES;
    *arg2 = YES;
    return Apple80211GetPower_second_orig(arg1, arg2);
}

static void stub(void) {
    assert(0 && "Stub should never be called");
}

void HookFunctionTests(void) {
    NSBundle *const bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/Preferences.framework"];
    assert(bundle != NULL);
    
    assert([bundle load]);

    // Hook C function with orig
    {
        int (*PSGetAirplaneMode)(void) = (__typeof__(PSGetAirplaneMode))dlsym(RTLD_DEFAULT, "PSGetAirplaneMode");
        assert(PSGetAirplaneMode != NULL);
        
        MicroInjectorReturn_t hookStatus = HookFunction((void *)PSGetAirplaneMode, (void *)PSGetAirplaneMode_hook, (void **)&PSGetAirplaneMode_orig);
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        assert(PSGetAirplaneMode() == 3);
    }

    // Hook C function no orig
    {
        int (*PSSetAirplaneMode)(int, int) = (__typeof__(PSSetAirplaneMode))dlsym(RTLD_DEFAULT, "PSSetAirplaneMode");
        assert(PSSetAirplaneMode != NULL);
        
        MicroInjectorReturn_t hookStatus = HookFunction((void *)PSSetAirplaneMode, (void *)PSSetAirplaneMode_hook, nil);
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        PSSetAirplaneMode(1, 2);

        assert(PSSetAirplaneMode_hook_called == YES);
    }

    // Re-hooking same function
    {
        int (*Apple80211GetPower)(int, BOOL *) = (__typeof__(Apple80211GetPower))dlsym(RTLD_DEFAULT, "Apple80211GetPower");
        assert(Apple80211GetPower != NULL);
        
        MicroInjectorReturn_t firstHookStatus = HookFunction((void *)Apple80211GetPower, (void *)Apple80211GetPower_first_hook, (void **)&Apple80211GetPower_first_orig);
        assert(firstHookStatus == MICROINJECTOR_SUCCESS);
        
        MicroInjectorReturn_t secondHookStatus = HookFunction((void *)Apple80211GetPower, (void *)Apple80211GetPower_second_hook, (void **)&Apple80211GetPower_second_orig);
        assert(secondHookStatus == MICROINJECTOR_SUCCESS);

        assert((void *)Apple80211GetPower_second_orig == (void *)Apple80211GetPower_first_hook);

        BOOL power = NO;
        int result = Apple80211GetPower(1, &power);

        assert(result == 1);
        assert(power == YES);

        assert(Apple80211GetPower_first_hook_called == YES);
        assert(Apple80211GetPower_second_hook_called == YES);
    }

    // Precondition failures
    {
        _Pragma("GCC diagnostic push");
        _Pragma("GCC diagnostic ignored \"-Wnonnull\"");

        int (*Apple80211SetPower)(int, BOOL) = (__typeof__(Apple80211SetPower))dlsym(RTLD_DEFAULT, "Apple80211SetPower");
        assert(Apple80211SetPower != NULL);

        {
            MicroInjectorReturn_t hookStatus = HookFunction((void *)Apple80211SetPower, nil, nil);
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }

        {
            MicroInjectorReturn_t hookStatus = HookFunction(nil, (void *)stub, nil);
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }

        _Pragma("GCC diagnostic pop");
    }
}
