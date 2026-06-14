#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <objc/runtime.h>

static BOOL PSBundleController_load_called = NO;
static void PSBundleController_load_hook(id self, SEL _cmd) {
    NSLog(@"Inside load hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    PSBundleController_load_called = YES;
}

static BOOL PSBundleController_unload_called = NO;

static void (*PSBundleController_unload_orig)(id, SEL) = NULL;
static void PSBundleController_unload_hook(id self, SEL _cmd) {
    PSBundleController_unload_orig(self, _cmd);
    
    NSLog(@"Inside unload hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    PSBundleController_unload_called = YES;
}

static BOOL PSListController_displaysButtonBar_hook(id self, SEL _cmd) {
    NSLog(@"Inside displaysButtonBar hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    return YES;
}

static BOOL (*PSViewController_isOverlay_orig)(id, SEL) = NULL;
static BOOL PSViewController_isOverlay_hook(id self, SEL _cmd) {
    NSLog(@"Inside isOverlay hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    return YES;
}

// static BOOL PSListController_reloadSpecifier_animated_called = NO;
// static void PSListController_reloadSpecifier_animated_hook(id self, SEL _cmd, PSSpecifier *specifier, BOOL animated) {
//     NSLog(@"Inside reloadSpecifier:animated: hook, self = %@, _cmd = %@, specifier = %@, animated = %@", self, NSStringFromSelector(_cmd), [specifier identifier], animated ? @"YES" : @"NO");
    
//     assert([specifier.identifier isEqualToString:@"Tests"]);
//     assert(animated == YES);

//     PSListController_reloadSpecifier_animated_called = YES;
// }

static NSString *PSListController_description_hook(id self, SEL _cmd) {
    NSLog(@"In description hook, self = %p, _cmd = %@", (void *)self, NSStringFromSelector(_cmd));
    return @"Hooked!";
}


static BOOL PSViewController_popController_first_hook_called = NO;
static BOOL (*PSViewController_popController_first_orig)(id, SEL) = NULL;
static BOOL PSViewController_popController_first_hook(__unused id self, __unused SEL _cmd) {
    PSViewController_popController_first_hook_called = YES;
    return YES;
}

static BOOL PSViewController_popController_second_hook_called = NO;
static BOOL (*PSViewController_popController_second_orig)(id, SEL) = NULL;
static BOOL PSViewController_popController_second_hook(id self, SEL _cmd) {
    PSViewController_popController_second_hook_called = YES;
    return PSViewController_popController_second_orig(self, _cmd);
}

static MicroInjectorTestReturnStruct_t PSViewController_structTest_nonhooked(__unused id self, __unused SEL _cmd) {
    return (MicroInjectorTestReturnStruct_t){
        .boolField = NO,
        .idField = nil,
        .intField = 24
    };
}

static MicroInjectorTestReturnStruct_t (*PSViewController_structTest_orig)(id, SEL) = NULL;
static MicroInjectorTestReturnStruct_t PSViewController_structTest_hook(id self, __unused SEL _cmd) {
    return (MicroInjectorTestReturnStruct_t){
        .boolField = YES,
        .idField = self,
        .intField = 42
    };
}

static void stub(void) {
    assert(0 && "This should never be called");
}

void HookMessageExTests(void) {
    NSBundle *const bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/Preferences.framework"];
    assert(bundle != NULL);
    
    assert([bundle load]);

    const Class PSBundleController_cls = NSClassFromString(@"PSBundleController");
    assert(PSBundleController_cls != NULL);

    Class const PSListController_cls = NSClassFromString(@"PSListController");
    assert(PSListController_cls != NULL);

    Class const PSViewController_cls = NSClassFromString(@"PSViewController");
    assert(PSViewController_cls != NULL);

    Class const PSSpecifier_cls = NSClassFromString(@"PSSpecifier");
    assert(PSSpecifier_cls != NULL);

    PSBundleController *const bundleController = [[PSBundleController_cls new] autorelease];
    assert(bundleController != NULL);

    PSListController *const listController = [[PSListController_cls new] autorelease];
    assert(listController != NULL);

    PSViewController *const viewController = [[PSViewController_cls new] autorelease];
    assert(viewController != NULL);

    // Instance method, no orig
    {
        IMP const originalImplementation = method_getImplementation(class_getInstanceMethod(object_getClass(bundleController), @selector(load)));
        assert(originalImplementation != NULL);

        HookMessageEx(PSBundleController_cls, @selector(load), (IMP)PSBundleController_load_hook, nil);

        IMP const hookedImplementation = method_getImplementation(class_getInstanceMethod(object_getClass(bundleController), @selector(load)));
        assert(hookedImplementation != NULL);

        assert(hookedImplementation != originalImplementation);

        [bundleController load];

        assert(PSBundleController_load_called);
    }

    // Instance method, with orig
    {
        IMP const originalImplementation = method_getImplementation(class_getInstanceMethod(object_getClass(bundleController), @selector(unload)));
        assert(originalImplementation != NULL);

        MicroInjectorReturn_t hookStatus = HookMessageEx(PSBundleController_cls, @selector(unload), (IMP)PSBundleController_unload_hook, (IMP *)&PSBundleController_unload_orig);
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        IMP const hookedImplementation = method_getImplementation(class_getInstanceMethod(object_getClass(bundleController), @selector(unload)));
        assert(hookedImplementation != NULL);

        assert(hookedImplementation != originalImplementation);
        assert((IMP)PSBundleController_unload_orig == originalImplementation);

        [bundleController unload];

        assert(PSBundleController_unload_called);
    }

    // Class method, no orig
    {
        IMP const originalImplementation = method_getImplementation(class_getClassMethod(PSListController_cls, @selector(displaysButtonBar)));
        assert(originalImplementation != NULL);

        MicroInjectorReturn_t hookStatus = HookMessageEx(object_getClass(PSListController_cls), @selector(displaysButtonBar), (IMP)PSListController_displaysButtonBar_hook, nil);
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        IMP const hookedImplementation = method_getImplementation(class_getClassMethod(PSListController_cls, @selector(displaysButtonBar)));
        assert(hookedImplementation != NULL);

        assert(hookedImplementation != originalImplementation);

        BOOL result = [PSListController_cls displaysButtonBar];
        assert(result == YES);
    }

    // Class method, with orig
    {

        IMP const originalImplementation = method_getImplementation(class_getClassMethod(PSViewController_cls, @selector(isOverlay)));
        assert(originalImplementation != NULL);

        MicroInjectorReturn_t hookStatus = HookMessageEx(object_getClass(PSViewController_cls), @selector(isOverlay), (IMP)PSViewController_isOverlay_hook, (IMP *)&PSViewController_isOverlay_orig);
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        IMP const hookedImplementation = method_getImplementation(class_getClassMethod(PSViewController_cls, @selector(isOverlay)));
        assert(hookedImplementation != NULL);

        assert(hookedImplementation != originalImplementation);

        BOOL result = [PSViewController_cls isOverlay];
        assert(result == YES);

        assert(PSViewController_isOverlay_orig(object_getClass(PSViewController_cls), @selector(isOverlay)) == NO);
    }

    // Superclass hook
    {
        IMP const originalImplementation = method_getImplementation(class_getInstanceMethod(PSListController_cls, @selector(description)));
        assert(originalImplementation != NULL);

        MicroInjectorReturn_t hookStatus = HookMessageEx(PSListController_cls, @selector(description), (IMP)PSListController_description_hook, nil);
        assert(hookStatus == MICROINJECTOR_SUCCESS);

        IMP const hookedImplementation = method_getImplementation(class_getInstanceMethod(PSListController_cls, @selector(description)));
        assert(hookedImplementation != NULL);

        assert(hookedImplementation != originalImplementation);

        NSString *const listControllerDescription = [listController description];
        NSString *const bundleControllerDescription = [bundleController description];

        NSString *const globalDescription = [[[NSObject new] autorelease] description];

        assert([listControllerDescription isEqualToString:@"Hooked!"]);
        assert(listControllerDescription != bundleControllerDescription);
        assert(![globalDescription isEqualToString:@"Hooked!"]);
    }

    // Re-hooking same selector twice
    {
        MicroInjectorReturn_t firstStatus = HookMessageEx(PSViewController_cls, @selector(popController), (IMP)PSViewController_popController_first_hook, (IMP *)&PSViewController_popController_first_orig);
        assert(firstStatus == MICROINJECTOR_SUCCESS);

        MicroInjectorReturn_t secondStatus = HookMessageEx(PSViewController_cls, @selector(popController), (IMP)PSViewController_popController_second_hook, (IMP *)&PSViewController_popController_second_orig);
        assert(secondStatus == MICROINJECTOR_SUCCESS);

        assert((IMP)PSViewController_popController_second_orig == (IMP)PSViewController_popController_first_hook);

        BOOL result = [viewController popController];

        assert(PSViewController_popController_first_hook_called == YES);
        assert(PSViewController_popController_second_hook_called == YES);
        assert(result == YES);
    }

    // Struct return
    {
        class_addMethod(PSViewController_cls, @selector(structTest), (IMP)PSViewController_structTest_nonhooked, "{MicroInjectorTestReturnStruct_t=i@i}");

        MicroInjectorReturn_t firstStatus = HookMessageEx(PSViewController_cls, @selector(structTest), (IMP)PSViewController_structTest_hook, (IMP *)&PSViewController_structTest_orig);
        assert(firstStatus == MICROINJECTOR_SUCCESS);

        assert((IMP)PSViewController_structTest_orig == (IMP)PSViewController_structTest_nonhooked);

        MicroInjectorTestReturnStruct_t originalResult = PSViewController_structTest_orig(viewController, @selector(structTest));
        assert(originalResult.boolField == NO);
        assert(originalResult.idField == nil);
        assert(originalResult.intField == 24);

        MicroInjectorTestReturnStruct_t hookedResult = [viewController structTest];
        assert(hookedResult.boolField == YES);
        assert(hookedResult.idField == viewController);
        assert(hookedResult.intField == 42);
    }

    // Hooking nonexistent selector
    {
        MicroInjectorReturn_t hookStatus = HookMessageEx(PSListController_cls, @selector(nonexistentSelector), (IMP)stub, nil);
        assert(hookStatus == MICROINJECTOR_METHOD_NOT_FOUND);
    }

    // Precondition failure
    {
        _Pragma("GCC diagnostic push");
        _Pragma("GCC diagnostic ignored \"-Wnonnull\"");

        {
            MicroInjectorReturn_t hookStatus = HookMessageEx(PSListController_cls, nil, (IMP)stub, nil);
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }
        
        {
            MicroInjectorReturn_t hookStatus = HookMessageEx(PSListController_cls, @selector(xyz), nil, nil);
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }

        {
            MicroInjectorReturn_t hookStatus = HookMessageEx(nil, @selector(xyz), (IMP)stub, nil);
            assert(hookStatus == MICROINJECTOR_PRECONDITION_FAILURE);
        }
        
        _Pragma("GCC diagnostic pop");
    }
}
