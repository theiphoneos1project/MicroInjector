#include "MicroInjector.h"
#include <objc/runtime.h>
#include <string.h>

void HookMessageEx(const Class klass, const SEL selector, IMP implementation, IMP *original) {
    if (klass == NULL || selector == NULL || implementation == NULL) {
        return;
    }

    Method const method = class_getInstanceMethod(klass, selector);
    if (method == NULL) {
        return;
    }

    if (original == NULL) {
        method_setImplementation(method, implementation);
    } else {
        IMP const oldImplementation = method_getImplementation(method);
        const char *const typeEncoding = method_getTypeEncoding(method);

        if (class_addMethod(klass, selector, implementation, typeEncoding)) {
            *original = oldImplementation;
        } else {
            *original = method_setImplementation(method, implementation);
        }
    }
}

IMP HookMessage(const Class klass, const SEL selector, IMP implementation, const char *prefix) {
    if (klass == NULL || selector == NULL || implementation == NULL) {
        return NULL;
    }

    Method const method = class_getInstanceMethod(klass, selector);
    if (method == NULL) {
        return NULL;
    }

    if (prefix != NULL) {
        const char *const name = sel_getName(selector);
        const char *const typeEncoding = method_getTypeEncoding(method);
        
        IMP const original = method_getImplementation(method);

        const size_t prefixLength = strlen(prefix);
        const size_t nameLength = strlen(name);

        char renamedName[prefixLength + nameLength + 1];
        bcopy(prefix, renamedName, prefixLength);
        bcopy(name, renamedName + prefixLength, nameLength + 1);

        class_addMethod(klass, sel_registerName(renamedName), original, typeEncoding);
        method_setImplementation(method, implementation);
        return NULL;
    }

    IMP original = NULL;
    HookMessageEx(klass, selector, implementation, &original);
    return original;
}
