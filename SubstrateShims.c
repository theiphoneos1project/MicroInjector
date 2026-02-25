#include "MicroInjector.h"

#define MI_EXPORT __attribute__((visibility("default"), used))

typedef const void *MSImageRef;

MI_EXPORT
void MSHookMessageEx(Class _class, SEL message, IMP hook, IMP *old) {
    (void)HookMessageEx(_class, message, hook, old);
}

MI_EXPORT
IMP MSHookMessage(Class _class, SEL sel, IMP imp, const char *prefix) {
    return HookMessage(_class, sel, imp, prefix);
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
