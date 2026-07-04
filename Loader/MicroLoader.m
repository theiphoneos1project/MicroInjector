//
// Copyright (c) 2026 Nightwind
//

#include <UIKit/UIKit.h>
#include <objc/runtime.h>
#include <mach-o/dyld.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <signal.h>
#include "../MicroInjector.h"
#include "MISafeModeAlert.h"

static NSString *g_mainExecutableName = nil;
static NSString *g_mainBundleIdentifier = nil;

static void SafeModeSignalHandler(__unused int signalNumber) {
    int fd = open(SAFE_MODE_MARKER_PATH, O_WRONLY | O_CREAT, 0644);
    
    if (fd >= 0) {
        close(fd);
    }

    _exit(1);
}

static void SafeModeExceptionHandler(__unused NSException *exception) {
    int fd = open(SAFE_MODE_MARKER_PATH, O_WRONLY | O_CREAT, 0644);
    
    if (fd >= 0) {
        close(fd);
    }

    _exit(1);
}

static void (*SpringBoard_applicationDidFinishLaunching_orig)(id, SEL, id) = NULL;
static void SpringBoard_applicationDidFinishLaunching_hook(id self, SEL _cmd, id application) {
    SpringBoard_applicationDidFinishLaunching_orig(self, _cmd, application);
    PresentSafeModeAlert();
}

static void (*SBStatusBarTimeView_drawRect_orig)(id, SEL, struct CGRect) = NULL;
static void SBStatusBarTimeView_drawRect_hook(id self, SEL _cmd, struct CGRect rect) {
    [self setValue:@"Safe Mode" forKey:@"_time"];
    SBStatusBarTimeView_drawRect_orig(self, _cmd, rect);
}

static void SBStatusBar_mouseDown__hook(__unused id self, __unused SEL _cmd, __unused void *event) {
    PresentSafeModeAlert();
}

static BOOL LibraryMatchesFilter(NSString *plistPath) {
    NSDictionary *const plist = [NSDictionary dictionaryWithContentsOfFile:plistPath];
    NSDictionary *const filter = [plist objectForKey:@"Filter"];

    if (filter == nil) {
        return YES;
    }

    for (NSString *bundle in [filter objectForKey:@"Bundles"]) {
        if ([bundle isEqualToString:g_mainBundleIdentifier]) {
            return YES;
        }
    }

    for (NSString *executable in [filter objectForKey:@"Executables"]) {
        if ([executable isEqualToString:g_mainExecutableName]) {
            return YES;
        }
    }

    for (NSString *cls in [filter objectForKey:@"Classes"]) {
        if (objc_getClass([cls UTF8String]) != nil) {
            return YES;
        }
    }

    return NO;
}

__attribute__((constructor)) static void init(void) {
    NSAutoreleasePool *const pool = [NSAutoreleasePool new];
    
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        g_mainExecutableName = [[NSString stringWithUTF8String:path] lastPathComponent];
    }
    
    g_mainBundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];

    const BOOL isSpringBoard = (g_mainExecutableName != NULL) && (strcmp(path, "/System/Library/CoreServices/SpringBoard.app/SpringBoard") == 0);

    if (isSpringBoard && access(SAFE_MODE_MARKER_PATH, F_OK) == 0) {
        (void)HookMessageEx(objc_getClass("SpringBoard"), @selector(applicationDidFinishLaunching:), (IMP)SpringBoard_applicationDidFinishLaunching_hook, (IMP *)&SpringBoard_applicationDidFinishLaunching_orig);
        (void)HookMessageEx(objc_getClass("SBStatusBarTimeView"), @selector(drawRect:), (IMP)SBStatusBarTimeView_drawRect_hook, (IMP *)&SBStatusBarTimeView_drawRect_orig);
        (void)HookMessageEx(objc_getClass("SBStatusBar"), @selector(mouseDown:), (IMP)SBStatusBar_mouseDown__hook, nil);
    } else {
        if (isSpringBoard) {
            const int signals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP };
            
            for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
                (void)signal(signals[i], SafeModeSignalHandler);
            }

            NSSetUncaughtExceptionHandler(&SafeModeExceptionHandler);
        }
        
        NSString *const dynamicLibrariesPath = [NSString stringWithUTF8String:"/Library/MicroInjector/DynamicLibraries"];

        NSFileManager *const fileManager = [NSFileManager defaultManager];
        NSArray *const fileNames = [[fileManager directoryContentsAtPath:dynamicLibrariesPath] sortedArrayUsingSelector:@selector(compare:)];

        if (fileNames) {
            for (NSString *const fileName in fileNames) {
                const NSRange range = [fileName rangeOfString:@".dylib"]; 
                if (range.location == NSNotFound || (range.location + range.length) != [fileName length]) {
                    continue;
                }

                NSString *const dylibPath = [dynamicLibrariesPath stringByAppendingPathComponent:fileName];
                NSString *const plistPath = [[dylibPath stringByDeletingPathExtension] stringByAppendingString:@".plist"];

                if (LibraryMatchesFilter(plistPath)) {
                    (void)dlopen([dylibPath UTF8String], RTLD_LAZY);
                }
            }
        }
    }

    [pool drain];
}
