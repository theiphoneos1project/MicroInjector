#include <stdio.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <spawn.h>

#define ARM_THREAD_STATE 1

const unsigned int LAUNCHD_PID = 1;
const char DYLIB_PATH[] = "/Library/MicroInjector/launchd_trampoline.dylib";
const size_t DYLIB_PATH_LENGTH = sizeof(DYLIB_PATH); // No need to compute at runtime

struct arm_thread_state {
    unsigned int registers[13];
    unsigned int sp;
    unsigned int lr;
    unsigned int pc;
    unsigned int cpsr;
};

int main(void) {
    void *const handle = dlopen(NULL, RTLD_LAZY);
    if (handle == NULL) {
        return 1;
    }

    const void *const real_dlopen = dlsym(handle, "dlopen");
    if (real_dlopen == NULL) {
        return 1;
    }

    const uintptr_t dlopen_addr = (uintptr_t)real_dlopen;
    
    kern_return_t kr;

    mach_port_t launchd_task = MACH_PORT_NULL;

    kr = task_for_pid(mach_task_self(), LAUNCHD_PID, &launchd_task);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] task_for_pid failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    thread_act_array_t threads = NULL;
    mach_msg_type_number_t thread_count = 0;
    kr = task_threads(launchd_task, &threads, &thread_count);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] task_threads failed: %s\n", mach_error_string(kr));
        return 1;
    }

    thread_act_t target_thread = threads[0];
    kr = thread_suspend(target_thread);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] thread_suspend failed: %s\n", mach_error_string(kr));
        return 1;
    }

    struct arm_thread_state orig_state = {0};
    mach_msg_type_number_t orig_state_count = sizeof(orig_state) / sizeof(unsigned int);
    kr = thread_get_state(target_thread, ARM_THREAD_STATE, (thread_state_t)&orig_state, &orig_state_count);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] thread_get_state failed: %s\n", mach_error_string(kr));
        return 1;
    }

    mach_vm_address_t path_addr = 0x0;
    kr = vm_allocate(launchd_task, (vm_address_t *)&path_addr, 0x1000, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_allocate path failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    kr = vm_write(launchd_task, (vm_address_t)path_addr, (vm_offset_t)DYLIB_PATH, DYLIB_PATH_LENGTH);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_write failed: %s\n", mach_error_string(kr));
        return 1;
    }

    mach_vm_address_t code_addr = 0x0;
    kr = vm_allocate(launchd_task, (vm_address_t *)&code_addr, 0x1000, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_allocate code failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    kr = vm_protect(launchd_task, (vm_address_t)code_addr, 0x1000, 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_protect failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    mach_vm_address_t data_addr = 0x0;
    kr = vm_allocate(launchd_task, (vm_address_t *)&data_addr, 0x1000, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_allocate data failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    const unsigned int data[] = {
        (unsigned int)orig_state.lr,
        (unsigned int)path_addr,
        dlopen_addr
    };
    
    kr = vm_write(launchd_task, (vm_address_t)data_addr, (vm_offset_t)data, sizeof(data));
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_write data failed: %s\n", mach_error_string(kr));
        return 1;
    }

    const unsigned int shellcode[] = {
        // r4 = start of argument data
        0xe59f4014, // ldr r4, [pc, #0x14]
        // path = r4 + 4
        0xe5940004, // ldr r0, [r4, #4]
        // r1 = RTLD_NOW
        0xe3a01002, // mov r1, #2
        // dlopen_addr = r4 + 8
        0xe594c008, // ldr ip, [r4, #8]
        // dlopen()
        0xe12fff3c, // blx ip
        // return to orig
        0xe3a00000, // mov r0, #0
        0xe594f000, // ldr pc, [r4]
        (unsigned int)data_addr
    };
    
    kr = vm_write(launchd_task, (vm_address_t)code_addr, (vm_offset_t)shellcode, sizeof(shellcode));
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] vm_write code failed: %s\n", mach_error_string(kr));
        return 1;
    }

    struct arm_thread_state new_state = orig_state;
    new_state.lr = (unsigned int)code_addr;

    mach_msg_type_number_t new_state_count = sizeof(new_state) / sizeof(unsigned int);

    kr = thread_set_state(target_thread, ARM_THREAD_STATE, (thread_state_t)&new_state, new_state_count);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] thread_set_state failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    kr = thread_resume(target_thread);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[-] thread_resume failed: %s\n", mach_error_string(kr));
        return 1;
    }
    
    // We have to kill SpringBoard manually in order to get it to inject
    pid_t pid;
    const char *args[] = { "killall", "SpringBoard", NULL };
    posix_spawn(&pid, "/usr/bin/killall", NULL, NULL, (char *const *)args, NULL);
    
    fprintf(stdout, "[+] Successfully injected dylib :)\n");
    
    return 0;
}
