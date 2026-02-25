#include "MicroInjector.h"
#include <objc/runtime.h>

#define MI_EXPORT __attribute__((visibility("default"), used))

typedef const void *MSImageRef;

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
