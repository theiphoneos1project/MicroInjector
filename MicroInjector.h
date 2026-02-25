#ifndef MICROINJECTOR_H
#define MICROINJECTOR_H

#include <objc/objc.h>
#include <mach/mach.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MICROINJECTOR_API __attribute__((visibility("default"), used))

#if defined(__clang__) && __has_feature(nullability)
#define MI_NONNULL _Nonnull
#define MI_NULLABLE _Nullable
#define MI_NULL_UNSPECIFIED _Null_unspecified
#else
#define MI_NONNULL
#define MI_NULLABLE
#define MI_NULL_UNSPECIFIED
#endif // defined(__clang__) && __has_feature(nullability)

typedef enum {
    MICROINJECTOR_SUCCESS = 0,

    MICROINJECTOR_PRECONDITION_FAILURE = 1,
    
    MICROINJECTOR_METHOD_NOT_FOUND = 2,

    MICROINJECTOR_MEMORY_HOOK_ALLOCATION_FAILURE = 3,
    MICROINJECTOR_MEMORY_HOOK_REMAP_FAILURE = 4,

    MICROINJECTOR_FUNCTION_HOOK_NO_VALID_TYPE = 5,
    MICROINJECTOR_FUNCTION_HOOK_TRAMPOLINE_ALLOCATION_FAILURE = 6,
    MICROINJECTOR_FUNCTION_HOOK_TRAMPOLINE_PROTECT_FAILURE = 7,
    MICROINJECTOR_FUNCTION_HOOK_PAGE_ALLOCATION_FAILURE = 8,
    MICROINJECTOR_FUNCTION_HOOK_REMAP_FAILURE = 9
} MicroInjectorReturn_t;

/**
 * Convert return type to string
 * @param code      A valid return type from a MicroInjector function
 * @return          A string representation of the error code
 */
MICROINJECTOR_API const char *MI_NONNULL MicroInjectorErrorToString(MicroInjectorReturn_t code);

/**
 * Hook Objective-C method.
 * @param cls               Class in which the selector resides
 * @param selector          Name of selector
 * @param implementation    New implementation of method
 * @param original          Pointer to original implementation
 * @return                  Status of hook
 */
MICROINJECTOR_API MicroInjectorReturn_t HookMessageEx(MI_NONNULL const Class cls, MI_NONNULL const SEL selector, MI_NONNULL IMP implementation, MI_NULLABLE IMP *MI_NULLABLE original);

/**
 * Hook arbitrary memory.
 * @param target    Target of memory to be hooked
 * @param data      Replacement memory
 * @param size      Size of memory
 * @return          Status of hook
 */
MICROINJECTOR_API MicroInjectorReturn_t HookMemory(void *MI_NONNULL const target, const void *const MI_NONNULL data, const size_t size);

/**
 * Hook C function.
 * @param target        Symbol to be hooked
 * @param replacement   Replacement hook C function
 * @param original      Pointer to original function
 * @return              Status of hook
 */
MICROINJECTOR_API MicroInjectorReturn_t HookFunction(void *MI_NONNULL const target, void *MI_NONNULL const replacement, void *MI_NULLABLE *MI_NULLABLE original);

typedef const void *LoadedImageReference;

/**
 * Find a loaded image by name.
 * @param name      Full path to binary on disk
 * @return          Reference to image or NULL if not found
 */
MICROINJECTOR_API MI_NULLABLE LoadedImageReference GetImageByName(const char *MI_NONNULL name);

/**
 * Find a symbol by name in a loaded image, walking the Mach-O table directly.
 * @param handle    Handle to a loaded image, or NULL to search all images
 * @param name      Symbol name with leading underscore (e.g. "_myFunction")
 * @return          Address of symbol or NULL if not found
 */
MICROINJECTOR_API void *MI_NULLABLE FindSymbol(MI_NULLABLE LoadedImageReference handle, const char *MI_NONNULL name);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MICROINJECTOR_H
