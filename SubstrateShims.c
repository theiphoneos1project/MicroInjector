#include "MicroInjector.h"
#include <objc/runtime.h>
#include <mach-o/loader.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MI_EXPORT __attribute__((visibility("default"), used))

static void hook_method_list(Class target, Class hook, Class old);

typedef const void *MSImageRef;
typedef struct mach_header MSImageHeader;

MI_EXPORT
void MSHookMessageEx(Class _class, SEL message, IMP hook, IMP *old) {
    (void)HookMessageEx(_class, message, hook, old);
}

MI_EXPORT
void MSHookMemory(void *target, const void *data, size_t size) {
    (void)HookMemory(target, data, size);
}

MI_EXPORT
void MSHookFunction(void *symbol, void *hook, void **old) {
    (void)HookFunction(symbol, hook, old);
}

MI_EXPORT
MSImageRef MSGetImageByName(const char *file) {
    return (MSImageRef)GetImageByName(file);
}

MI_EXPORT
void *MSFindSymbol(MSImageRef image, const char *name) {
    return FindSymbol((LoadedImageReference)image, name);
}

MI_EXPORT
IMP MSHookMessage(Class _class, SEL sel, IMP imp, const char *prefix) {
    if (_class == NULL || sel == NULL || imp == NULL) {
        return NULL;
    }

    Method const method = class_getInstanceMethod(_class, sel);
    if (method == NULL) {
        return NULL;
    }

    if (prefix != NULL) {
        const char *const name = sel_getName(sel);
        const char *const typeEncoding = method_getTypeEncoding(method);
        
        IMP const original = method_getImplementation(method);

        const size_t prefixLength = strlen(prefix);
        const size_t nameLength = strlen(name);

        char renamedName[prefixLength + nameLength + 1];
        bcopy(prefix, renamedName, prefixLength);
        bcopy(name, renamedName + prefixLength, nameLength + 1);

        class_addMethod(_class, sel_registerName(renamedName), original, typeEncoding);
        method_setImplementation(method, imp);
        return NULL;
    }

    IMP original = NULL;
    HookMessageEx(_class, sel, imp, &original);
    return original;
}

MI_EXPORT
void MSHookClassPair(Class target, Class hook, Class old) {
    if (target == NULL || hook == NULL || old == NULL) {
        return;
    }

    // Instance methods
    hook_method_list(target, hook, old);
    // Class methods
    hook_method_list(object_getClass((id)target), object_getClass((id)hook), object_getClass((id)old));
}

/* === Non-implemented functions === */

MI_EXPORT
MSImageRef MSMapImage(const char *file) {
    fprintf(stderr, "MicroInjector: MSMapImage is not implemented\n");
    __builtin_trap();
    return NULL;
}

MI_EXPORT
const MSImageHeader *MSImageAddress(MSImageRef image) {
    fprintf(stderr, "MicroInjector: MSImageAddress is not implemented\n");
    __builtin_trap();
    return NULL;
}

MI_EXPORT
void MSCloseImage(MSImageRef image) {
    fprintf(stderr, "MicroInjector: MSCloseImage is not implemented\n");
    __builtin_trap();
}

MI_EXPORT
char *MSFindAddress(MSImageRef image, void **address) {
    fprintf(stderr, "MicroInjector: MSFindAddress is not implemented\n");
    __builtin_trap();
    return NULL;
}

MI_EXPORT
bool MSHookProcess(pid_t pid, const char *library) {
    fprintf(stderr, "MicroInjector: MSHookProcess is not implemented\n");
    __builtin_trap();
    return false;
}

/* === Private utility functions === */

static void hook_method_list(Class target, Class hook, Class old) {
    unsigned int methodCount = 0;
    Method *const methods = class_copyMethodList(hook, &methodCount);
    
    for (unsigned int i = 0; i < methodCount; i++) {
        const Method method = methods[i];
        const SEL selector = method_getName(method);
        const IMP hookImplementation = method_getImplementation(method);
        const char *const typeEncoding = method_getTypeEncoding(method);
        
        const Method original = class_getInstanceMethod(target, selector);
        if (original != NULL) {
            class_addMethod(old, selector, method_getImplementation(original), typeEncoding);
        } 
        
        if (!class_addMethod(target, selector, hookImplementation, typeEncoding)) {
            method_setImplementation(original, hookImplementation);
        }
    }

    free((void *)methods);
}
