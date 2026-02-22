#ifndef MICROINJECTOR_H
#define MICROINJECTOR_H

#include <objc/objc.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(clang)
#if __has_feature(nullability)
#define MI_NONNULL _Nonnull
#define MI_NULLABLE _Nullable
#define MI_NULL_UNSPECIFIED _Null_unspecified
#else
#define MI_NONNULL
#define MI_NULLABLE
#define MI_NULL_UNSPECIFIED
#endif // __has_feature(nullability)
#else
#define MI_NONNULL
#define MI_NULLABLE
#define MI_NULL_UNSPECIFIED
#endif // defined(__has_feature)

void HookMessageEx(MI_NONNULL const Class klass, MI_NONNULL const SEL selector, MI_NONNULL IMP implementation, MI_NULLABLE IMP *MI_NULLABLE original);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MICROINJECTOR_H
