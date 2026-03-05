#include <stdint.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "../MicroInjector.h"

static int (*execve_orig)(const char *, char *const[], char *const[]) = NULL;
static int execve_hook(const char *path, char *const argv[], char *const envp[]) {
    if (path == NULL) {
        return execve_orig(path, argv, envp);
    }

    if (strcmp(path, "/System/Library/CoreServices/SpringBoard.app/SpringBoard") != 0) {
        return execve_orig(path, argv, envp);
    }

    unsigned int envc = 0;
    
    if (envp != NULL) {
        while (envp[envc] != NULL) {
            envc += 1;
        }
    }

    char *newenv[envc + 2];
    for (unsigned int i = 0; i < envc; i++) {
        newenv[i] = envp[i];
    }

    newenv[envc++] = "DYLD_INSERT_LIBRARIES=/Library/MicroInjector/MicroLoader.dylib";
    newenv[envc] = NULL;

    return execve_orig(path, argv, newenv);
}

__attribute__((constructor)) static void init(void) {
    void *const handle = dlopen("/usr/lib/libSystem.B.dylib", RTLD_NOW);
    if (handle == NULL) {
        return;
    }
    
    void *symbol = dlsym(handle, "execve");
    if (symbol == NULL) {
        return;
    }
    
    (void)HookFunction(symbol, (void *)execve_hook, (void **)&execve_orig);
}
