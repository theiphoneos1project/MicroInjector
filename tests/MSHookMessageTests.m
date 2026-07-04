//
// Copyright (c) 2026 Nightwind
//

#include "../MicroInjector.h"
#include "Private.h"
#include <Foundation/Foundation.h>
#include <objc/runtime.h>

extern IMP MSHookMessage(Class _class, SEL sel, IMP imp, const char *prefix);

static int PhotosNavigationItem_barStyle_hook(id self, SEL _cmd) {
    NSLog(@"In barStyle hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    return 3;
}

static NSBundle *UIApplication_pickerBundle_hook(id self, SEL _cmd) {
    NSLog(@"In pickerBundle hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    return [NSBundle bundleWithPath:@"/System/Library/Frameworks/Preferences.framework"];
}

static BOOL PhotosNavigationItem_setBarStyle_hook_called = NO;
static void PhotosNavigationItem_setBarStyle_hook(id self, SEL _cmd, int style) {
    NSLog(@"In barStyle hook, self = %@, _cmd = %@, style = %d", self, NSStringFromSelector(_cmd), style);
    PhotosNavigationItem_setBarStyle_hook_called = YES;
}

static BOOL UIApplication_setPickerBundle_hook_called = NO;
static void UIApplication_setPickerBundle_hook(id self, SEL _cmd, NSBundle *bundle) {
    NSLog(@"In pickerBundle hook, self = %@, _cmd = %@, bundle = %@", self, NSStringFromSelector(_cmd), bundle);
    UIApplication_setPickerBundle_hook_called = YES;
}

static NSString *PhotosNavigationItem_description_hook(__unused id self, __unused SEL _cmd) {
    return @"Hooked!";
}

static BOOL BackgroundView_popNavigationItem_hook_called = NO;
static void BackgroundView_popNavigationItem_hook(id self, SEL _cmd) {
    NSLog(@"In popNavigationItem hook, self = %@, _cmd = %@", self, NSStringFromSelector(_cmd));
    BackgroundView_popNavigationItem_hook_called = YES;
}

static void stub(void) {
    assert(0 && "Stub should never be called");
}

void MSHookMessageTests(void) {
    NSBundle *const bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/PhotoLibrary.framework"];
    assert(bundle != NULL);
    
    assert([bundle load]);

    const Class PhotosNavigationItem_cls = NSClassFromString(@"PhotosNavigationItem");
    assert(PhotosNavigationItem_cls != NULL);

    const Class UIApplication_cls = NSClassFromString(@"UIApplication");
    assert(UIApplication_cls != NULL);

    const Class BackgroundView_cls = NSClassFromString(@"BackgroundView");
    assert(BackgroundView_cls != NULL);

    const Class PLImageScroller_cls = NSClassFromString(@"PLImageScroller");
    assert(PLImageScroller_cls != NULL);

    PhotosNavigationItem *const navigationItem = [[PhotosNavigationItem_cls new] autorelease];
    assert(navigationItem != NULL);

    BackgroundView *const backgroundView = [[BackgroundView_cls new] autorelease];
    assert(backgroundView != NULL);

    PLImageScroller *const imageScroller = [[PLImageScroller_cls new] autorelease];
    assert(imageScroller != NULL);

    // Instance method with prefix
    {
        const SEL selector = @selector(barStyle);
        const char *const prefix = "mi_";

        const Method method = class_getInstanceMethod(PhotosNavigationItem_cls, selector);
        assert(method != NULL);

        const IMP originalImplementation = method_getImplementation(method);
        assert(originalImplementation != NULL);

        assert(MSHookMessage(PhotosNavigationItem_cls, selector, (IMP)PhotosNavigationItem_barStyle_hook, prefix) == NULL);

        const SEL renamedSelector = NSSelectorFromString([NSString stringWithFormat:@"%s%s", prefix, sel_getName(selector)]);

        const Method renamedMethod = class_getInstanceMethod(PhotosNavigationItem_cls, renamedSelector);
        assert(renamedMethod != NULL);

        const IMP renamedImplementation = method_getImplementation(renamedMethod);
        assert(renamedImplementation == originalImplementation);

        const Method hookedMethod = class_getInstanceMethod(PhotosNavigationItem_cls, selector);
        assert(hookedMethod != NULL);

        const IMP newImplementation = method_getImplementation(hookedMethod);
        assert(newImplementation == (IMP)PhotosNavigationItem_barStyle_hook);

        assert([navigationItem barStyle] == 3);
    }

    // Class method with prefix
    {
        const SEL selector = @selector(pickerBundle);
        const char *const prefix = "mi_";

        const Method method = class_getInstanceMethod(object_getClass(UIApplication_cls), selector);
        assert(method != NULL);

        const IMP originalImplementation = method_getImplementation(method);
        assert(originalImplementation != NULL);

        assert(MSHookMessage(object_getClass(UIApplication_cls), selector, (IMP)UIApplication_pickerBundle_hook, prefix) == NULL);

        const SEL renamedSelector = NSSelectorFromString([NSString stringWithFormat:@"%s%s", prefix, sel_getName(selector)]);

        const Method renamedMethod = class_getInstanceMethod(object_getClass(UIApplication_cls), renamedSelector);
        assert(renamedMethod != NULL);

        const IMP renamedImplementation = method_getImplementation(renamedMethod);
        assert(renamedImplementation == originalImplementation);

        const Method hookedMethod = class_getInstanceMethod(object_getClass(UIApplication_cls), selector);
        assert(hookedMethod != NULL);

        const IMP newImplementation = method_getImplementation(hookedMethod);
        assert(newImplementation == (IMP)UIApplication_pickerBundle_hook);

        assert([[[UIApplication_cls pickerBundle] bundlePath] isEqualToString:@"/System/Library/Frameworks/Preferences.framework"]);
    }

    // Instance method no prefix
    {
        const SEL selector = @selector(setBarStyle:);

        const Method method = class_getInstanceMethod(PhotosNavigationItem_cls, selector);
        assert(method != NULL);

        const IMP originalImplementation = method_getImplementation(method);
        assert(originalImplementation != NULL);

        const IMP returnedImplementation = MSHookMessage(PhotosNavigationItem_cls, selector, (IMP)PhotosNavigationItem_setBarStyle_hook, nil);

        assert(returnedImplementation == originalImplementation);

        const Method hookedMethod = class_getInstanceMethod(PhotosNavigationItem_cls, selector);
        assert(hookedMethod != NULL);

        const IMP newImplementation = method_getImplementation(hookedMethod);
        assert(newImplementation == (IMP)PhotosNavigationItem_setBarStyle_hook);

        [navigationItem setBarStyle:5];
        
        assert(PhotosNavigationItem_setBarStyle_hook_called == YES);
    }

    // Class method no prefix
    {
        const SEL selector = @selector(setPickerBundle:);

        const Method method = class_getInstanceMethod(object_getClass(UIApplication_cls), selector);
        assert(method != NULL);

        const IMP originalImplementation = method_getImplementation(method);
        assert(originalImplementation != NULL);

        const IMP returnedImplementation = MSHookMessage(object_getClass(UIApplication_cls), selector, (IMP)UIApplication_setPickerBundle_hook, nil);

        assert(returnedImplementation == originalImplementation);

        const Method hookedMethod = class_getInstanceMethod(object_getClass(UIApplication_cls), selector);
        assert(hookedMethod != NULL);

        const IMP newImplementation = method_getImplementation(hookedMethod);
        assert(newImplementation == (IMP)UIApplication_setPickerBundle_hook);

        [UIApplication_cls setPickerBundle:nil];
        
        assert(UIApplication_setPickerBundle_hook_called == YES);
    }

    // Superclass hook
    {
        const SEL selector = @selector(description);
        const char *const prefix = "mi_"; 

        const Class subclass = PhotosNavigationItem_cls; 

        const Class superclass = class_getSuperclass(subclass); 
        assert(superclass != NULL);

        assert(MSHookMessage(subclass, selector, (IMP)PhotosNavigationItem_description_hook, prefix) == NULL);

        NSString *const description = [navigationItem description];
        id superInstance = [[superclass new] autorelease];
        NSString *const superDescription = [superInstance description];

        assert([description isEqualToString:@"Hooked!"]);
        assert(![superDescription isEqualToString:@"Hooked!"]);
    }

    // Inherited method
    {
        const SEL selector = @selector(hash);
        const char *const prefix = "mi_"; 

        const Class subclass = PhotosNavigationItem_cls; 
        
        const Class superclass = class_getSuperclass(subclass); 
        assert(superclass != NULL);

        const Method superMethodBefore = class_getInstanceMethod(superclass, selector);
        assert(superMethodBefore != NULL);

        const IMP superImplementationBefore = method_getImplementation(superMethodBefore);
        assert(superImplementationBefore != NULL);

        assert(MSHookMessage(subclass, selector, (IMP)stub, prefix) == NULL);

        const Method superMethodAfter = class_getInstanceMethod(superclass, selector);
        assert(superMethodAfter != NULL);

        const IMP superImplementationAfter = method_getImplementation(superMethodAfter);
        assert(superImplementationAfter != NULL);

        assert(superImplementationBefore == superImplementationAfter);
    }

    // Double hook
    {
        const SEL selector = @selector(popNavigationItem);
        const char *const prefix = "mi_";

        assert(MSHookMessage(BackgroundView_cls, selector, (IMP)BackgroundView_popNavigationItem_hook, prefix) == NULL);
        assert(MSHookMessage(BackgroundView_cls, selector, (IMP)BackgroundView_popNavigationItem_hook, prefix) == NULL);

        [backgroundView popNavigationItem];

        assert(BackgroundView_popNavigationItem_hook_called == YES);
    }

    // Calling original
    {
        const SEL selector = @selector(canHandleSwipes);
        const char *const prefix = "mi_";

        MSHookMessage(PLImageScroller_cls, selector, (IMP)stub, prefix);

        BOOL canHandleSwipes = [imageScroller mi_canHandleSwipes];
        assert(canHandleSwipes == NO);
    }

    // Precondition failures
    {
        assert(MSHookMessage(NULL, @selector(xyz), (IMP)stub, NULL) == NULL);
        assert(MSHookMessage(PLImageScroller_cls, NULL, (IMP)stub, NULL) == NULL);
        assert(MSHookMessage(PLImageScroller_cls, @selector(xyz), NULL, NULL) == NULL);
    }
}
