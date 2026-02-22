#ifndef MICROINJECTOR_H
#define MICROINJECTOR_H

#include <objc/objc.h>
#include <mach/mach.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(__clang__) && __has_feature(nullability)
#define MI_NONNULL _Nonnull
#define MI_NULLABLE _Nullable
#define MI_NULL_UNSPECIFIED _Null_unspecified
#else
#define MI_NONNULL
#define MI_NULLABLE
#define MI_NULL_UNSPECIFIED
#endif // defined(__clang__) && __has_feature(nullability)

void HookMessageEx(MI_NONNULL const Class klass, MI_NONNULL const SEL selector, MI_NONNULL IMP implementation, MI_NULLABLE IMP *MI_NULLABLE original);

__attribute__((deprecated(
    "HookMessage is deprecated and is only implementated for backwards-compatibility. "
    "DEVELOPERS SHOULD NOT USE THIS API. More info: https://www.cydiasubstrate.com/api/c/MSHookMessage/"
)))
MI_NULLABLE IMP HookMessage(MI_NONNULL const Class klass, MI_NONNULL SEL selector, MI_NONNULL IMP implementation, const char *MI_NULLABLE prefix); 

kern_return_t HookMemory(void *MI_NONNULL const target, const void *const MI_NONNULL data, const size_t size);

typedef const void *LoadedImageReference;
MI_NULLABLE LoadedImageReference GetImageByName(const char *MI_NONNULL name);

/**
 * Find a symbol by name in a loaded image, walking the Mach-O table directly.
 * @param handle    Handle to a loaded image, or NULL to search all images
 * @param name      Symbol name with leading underscore (e.g. "_myFunction")
 * @return          Address of symbol or NULL if not found
 */
void *MI_NULLABLE FindSymbol(MI_NULLABLE LoadedImageReference handle, const char *MI_NONNULL name);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MICROINJECTOR_H
