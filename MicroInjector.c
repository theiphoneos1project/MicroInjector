#include "MicroInjector.h"
#include <objc/runtime.h>

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
