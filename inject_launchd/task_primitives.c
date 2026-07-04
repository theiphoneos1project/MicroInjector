//
// Thanks to staturnz for the huge amount of help!
//

#include "task_primitives.h"
#include <stdbool.h>

uint32_t task_allocate(mach_port_t task, uint32_t size) {
    vm_address_t address = 0;
    
    if (vm_allocate(task, &address, size, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) {
        return 0;
    }

    return (uint32_t)address;
}

kern_return_t task_deallocate(mach_port_t task, uint32_t address, size_t size) {
    return vm_deallocate(task, address, size);
}

kern_return_t task_protect(mach_port_t task, uint32_t address, uint32_t size, vm_prot_t protectionFlags) {
    vm_protect(task, address, size, true, protectionFlags);
    return vm_protect(task, address, size, false, protectionFlags);
}

kern_return_t task_write(mach_port_t task, uint32_t address, void *buffer, uint32_t size) {
    return vm_write(task, (vm_address_t)address, (vm_offset_t)buffer, (mach_msg_type_number_t)size);
}
