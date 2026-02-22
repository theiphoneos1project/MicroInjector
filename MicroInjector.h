#ifndef MICROINJECTOR_H
#define MICROINJECTOR_H

#include <objc/objc.h>

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

__deprecated_msg(
    "HookMessage is deprecated and is only implementated for backwards-compatibility. "
    "DEVELOPERS SHOULD NOT USE THIS API. More info: https://www.cydiasubstrate.com/api/c/MSHookMessage/"
)
MI_NULLABLE IMP HookMessage(MI_NONNULL const Class klass, MI_NONNULL SEL selector, MI_NONNULL IMP implementation, const char *MI_NULLABLE prefix); 

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MICROINJECTOR_H
