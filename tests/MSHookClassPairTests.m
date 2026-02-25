#include "Private.h"
#include <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <dlfcn.h>

extern void MSHookClassPair(Class target, Class hook, Class old);

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wincomplete-implementation\"")

@interface CPActivityMonitor_hook : NSObject
+ (id)currentMonitor;
- (BOOL)shouldCancel;
- (NSString *)mi_testMethod;
@end

@interface CPActivityMonitor_old : NSObject
+ (id)currentMonitor;
- (BOOL)shouldCancel;
- (NSString *)mi_testMethod;
@end

@implementation CPActivityMonitor_hook

- (BOOL)shouldCancel {
    return YES;
}

+ (id)currentMonitor {
    return @"Hooked currentMonitor";
}

- (NSString *)mi_testMethod {
    return @"Hooked!";
}

@end

@implementation CPActivityMonitor_old
@end

@interface CPURLMatch_hook : NSObject
- (NSRange)range;
- (NSString *)description;
@end

@interface CPURLMatch_old : NSObject
- (NSRange)range;
- (NSString *)description;
@end

@implementation CPURLMatch_hook

- (NSRange)range {
    return NSMakeRange(99, 99);
}

- (NSString *)description {
    return @"Hooked description!";
}

@end

@implementation CPURLMatch_old
@end

@interface CPURLMatch_empty_hook : NSObject
@end

@interface CPURLMatch_empty_old : NSObject
@end

@implementation CPURLMatch_empty_hook
@end

@implementation CPURLMatch_empty_old
@end

_Pragma("GCC diagnostic pop")

void MSHookClassPairTests(void) {
    NSBundle *const bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/URLify.framework"];
    assert(bundle != NULL);
    assert([bundle load]);
    
    const Class CPActivityMonitor_cls = NSClassFromString(@"CPActivityMonitor");
    assert(CPActivityMonitor_cls != NULL);
    
    const Class CPURLMatch_cls = NSClassFromString(@"CPURLMatch");
    assert(CPURLMatch_cls != NULL);

    assert(class_getInstanceMethod(CPActivityMonitor_cls, @selector(mi_testMethod)) == NULL);
    
    MSHookClassPair(CPActivityMonitor_cls, [CPActivityMonitor_hook class], [CPActivityMonitor_old class]);
    MSHookClassPair(CPURLMatch_cls, [CPURLMatch_hook class], [CPURLMatch_old class]);

    // Instance method hook
    {
        CPActivityMonitor *const monitor = [[CPActivityMonitor_cls new] autorelease];
        assert(monitor != NULL);
        assert([monitor shouldCancel] == YES);
    }

    // Class method hook
    {
        assert([[CPActivityMonitor_cls currentMonitor] isEqualToString:@"Hooked currentMonitor"]);
    }

    // Struct return hook
    {
        CPURLMatch *const match = [[CPURLMatch_cls new] autorelease];
        assert(match != NULL);

        const NSRange range = [match range];
        assert(range.location == 99);
        assert(range.length == 99);
    }

    // New method
    {
        assert(class_getInstanceMethod(CPActivityMonitor_cls, @selector(mi_testMethod)) != NULL);
        
        CPActivityMonitor *const monitor = [[CPActivityMonitor_cls new] autorelease];
        assert(monitor != NULL);
        
        NSString *const testMethodResult = [(CPActivityMonitor_hook *)monitor mi_testMethod];
        assert([testMethodResult isEqualToString:@"Hooked!"]);
    }

    // Re-hooking same method
    {
        MSHookClassPair(CPActivityMonitor_cls, [CPActivityMonitor_hook class], [CPActivityMonitor_old class]);
        
        CPActivityMonitor *const monitor = [[CPActivityMonitor_cls new] autorelease];
        assert(monitor != NULL);

        assert([monitor shouldCancel] == YES);
        assert([[CPActivityMonitor_cls currentMonitor] isEqualToString:@"Hooked currentMonitor"]);
    }

    // Empty hook
    {
        unsigned int methodCountBefore = 0;
        Method *methods = class_copyMethodList(CPURLMatch_cls, &methodCountBefore);
        free(methods);
        
        const IMP descriptionBefore = method_getImplementation(class_getInstanceMethod(CPURLMatch_cls, @selector(description)));
        
        MSHookClassPair(CPURLMatch_cls, [CPURLMatch_empty_hook class], [CPURLMatch_empty_old class]);
        
        unsigned int methodCountAfter = 0;
        methods = class_copyMethodList(CPURLMatch_cls, &methodCountAfter);
        free(methods);
        
        const IMP descriptionAfter = method_getImplementation(class_getInstanceMethod(CPURLMatch_cls, @selector(description)));
        
        assert(methodCountBefore == methodCountAfter);
        assert(descriptionBefore == descriptionAfter);
    }

    // Precondition failures
    {
        const Method shouldCancelMethod = class_getInstanceMethod(CPActivityMonitor_cls, @selector(shouldCancel));
        assert(shouldCancelMethod != NULL);

        const Method currentMonitorMethod = class_getInstanceMethod(object_getClass(CPActivityMonitor_cls), @selector(currentMonitor));
        assert(currentMonitorMethod != NULL);

        const IMP shouldCancelBefore = method_getImplementation(shouldCancelMethod);
        assert(shouldCancelBefore != NULL);

        const IMP currentMonitorBefore = method_getImplementation(currentMonitorMethod);
        assert(currentMonitorBefore != NULL);

        unsigned int methodCountBefore = 0;
        Method *methods = class_copyMethodList(CPActivityMonitor_cls, &methodCountBefore);
        free(methods);

        MSHookClassPair(nil, [CPActivityMonitor_hook class], [CPActivityMonitor_old class]);
        MSHookClassPair(CPActivityMonitor_cls, nil, [CPActivityMonitor_old class]);
        MSHookClassPair(CPActivityMonitor_cls, [CPActivityMonitor_hook class], nil);
        
        const IMP shouldCancelAfter = method_getImplementation(shouldCancelMethod);
        assert(shouldCancelAfter != NULL);

        const IMP currentMonitorAfter = method_getImplementation(currentMonitorMethod);
        assert(currentMonitorAfter != NULL);

        unsigned int methodCountAfter = 0;
        methods = class_copyMethodList(CPActivityMonitor_cls, &methodCountAfter);
        free(methods);

        assert(shouldCancelBefore == shouldCancelAfter);
        assert(currentMonitorBefore == currentMonitorAfter);
        assert(methodCountBefore == methodCountAfter);
    }
}
