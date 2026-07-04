//
// Thanks to staturnz for the huge amount of help!
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <spawn.h>

#include "task_primitives.h"

extern void __pthread_set_self(void);

static const unsigned int LAUNCHD_PID = 1;
static const char DYLIB_PATH[] = "/Library/MicroInjector/launchd_trampoline.dylib";

static const uint32_t SHELLCODE_SIZE = 0x1000;
static const uint32_t STACK_SIZE = 0x8000;

static const uint32_t SHELLCODE_BASE[] = {
    0xe28f80bc, 0xe598c000, 0xe28f00b0, 0xe5900000,
    0xe12fff3c, 0xee1d0f70, 0xe3b01000, 0xe1500001,
    0x0a000004, 0xee1d0f70, 0xe3c00003, 0xe28f108c,
    0xe5911000, 0xe5801000, 0xe28f008c, 0xe3b01001,
    0xe28f8080, 0xe598c000, 0xe12fff3c, 0xe3b00000,
    0xe320f000, 0xe24dd040, 0xe3e0c01a, 0xef000080,
    0xe58d0008, 0xe3e0c019, 0xef000080, 0xe58d000c,
    0xe1a04000, 0xe28f0040, 0xe5900000, 0xe58d0000,
    0xe3a00018, 0xe58d0004, 0xe3a00000, 0xe58d0010,
    0xe3a00ee1, 0xe58d0014, 0xe1a0000d, 0xe3a01003,
    0xe3a02018, 0xe3a0302c, 0xe3a05000, 0xe3a06000,
    0xe3e0c01e, 0xef000080, 0xe7ffdefe, 0x00001511,
    0x41414141, 0x42424242, 0x43434343
};

int main(void) {
    int status = EXIT_FAILURE;
    uint8_t *shellcodeData = NULL;
    
    kern_return_t kr;
    
    mach_port_t launchdTask = MACH_PORT_NULL;
    
    kr = task_for_pid(mach_task_self(), LAUNCHD_PID, &launchdTask);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] task_for_pid failed: %s\n", mach_error_string(kr));
        goto done;
    }
    
    uint32_t shellcodeAddress = task_allocate(launchdTask, SHELLCODE_SIZE);
    if (shellcodeAddress == 0) {
        fprintf(stderr, "[-] Failed to allocate shellcode in launchd\n");
        goto done;
    }
    
    uint32_t stackAddress = task_allocate(launchdTask, STACK_SIZE);
    if (stackAddress == 0) {
        fprintf(stderr, "[-] Failed to allocate stack in launchd\n");
        goto done;
    }
    
    shellcodeData = calloc(1, SHELLCODE_SIZE);
    if (shellcodeData == NULL) {
        fprintf(stderr, "[-] Failed to calloc shellcode data\n");
        goto done;
    }
    
    memcpy(shellcodeData, SHELLCODE_BASE, sizeof(SHELLCODE_BASE));
    *(uint32_t *)(shellcodeData + 0xc0) = stackAddress + 0x40;
    *(uint32_t *)(shellcodeData + 0xc4) = (uint32_t)__pthread_set_self;
    *(uint32_t *)(shellcodeData + 0xc8) = (uint32_t)dlopen;
    strcpy((char *)(shellcodeData + 0xcc), DYLIB_PATH);
    
    kr = task_write(launchdTask, shellcodeAddress, shellcodeData, SHELLCODE_SIZE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] Failed to write shellcode data to shellcode address: %s\n", mach_error_string(kr));
        goto done;
    }
    
    kr = task_protect(launchdTask, shellcodeAddress, SHELLCODE_SIZE, VM_PROT_READ | VM_PROT_EXECUTE | VM_PROT_COPY);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] Failed to protect shellcode: %s\n", mach_error_string(kr));
        goto done;
    }
    
    thread_t thread = MACH_PORT_NULL;
    
    kr = thread_create(launchdTask, &thread);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] Failed to create thread: %s\n", mach_error_string(kr));
        goto done;
    }
    
    arm_thread_state_t state = {0};
    mach_msg_type_number_t count = ARM_THREAD_STATE_COUNT;

    kr = thread_get_state(thread, ARM_THREAD_STATE, (thread_state_t)&state, &count);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] Failed to get thread state: %s\n", mach_error_string(kr));
        goto done;
    }
    
    state.__cpsr = 0x60000010;
    state.__pc = shellcodeAddress;
    state.__sp = stackAddress + 0x4000;
    
    kr = thread_set_state(thread, ARM_THREAD_STATE, (thread_state_t)&state, count);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] Failed to set thread state: %s\n", mach_error_string(kr));
        goto done;
    }
    
    kr = thread_resume(thread);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] Failed to resume thread: %s\n", mach_error_string(kr));
        goto done;
    }

    usleep(250000);

    thread = MACH_PORT_NULL;
    status = EXIT_SUCCESS;

    // We have to kill SpringBoard manually in order to get it to inject
    pid_t pid;
    const char *args[] = { "killall", "SpringBoard", NULL };
    posix_spawn(&pid, "/usr/bin/killall", NULL, NULL, (char *const *)args, NULL);
    
    fprintf(stdout, "[+] Successfully injected dylib :)\n");

done:
    if (shellcodeData != NULL) {
        free(shellcodeData);
    }

    if (status == 0) {
        mach_port_deallocate(mach_task_self(), launchdTask);
        return 0;
    }

    if (MACH_PORT_VALID(launchdTask)) {
        if (shellcodeAddress != 0) {
            kr = task_deallocate(launchdTask, shellcodeAddress, SHELLCODE_SIZE);
            if (kr != KERN_SUCCESS) {
                fprintf(stderr, "[-] Failed to deallocate shellcode: %s\n", mach_error_string(kr));
            }
        }

        if (stackAddress != 0) {
            kr = task_deallocate(launchdTask, stackAddress, STACK_SIZE);
            if (kr != KERN_SUCCESS) {
                fprintf(stderr, "[-] Failed to deallocate stack: %s\n", mach_error_string(kr));
            }
        }

        if (MACH_PORT_VALID(thread)) {
            kr = thread_terminate(thread);
            if (kr != KERN_SUCCESS) {
                fprintf(stderr, "[-] Failed to terminate thread: %s\n", mach_error_string(kr));
            }
        }

        kr = mach_port_deallocate(mach_task_self(), launchdTask);
        if (kr != KERN_SUCCESS) {
            fprintf(stderr, "[-] Failed to deallocate task: %s\n", mach_error_string(kr));
        }
    }

    return EXIT_FAILURE;
}
