//
// Thanks to staturnz for the huge amount of help!
//

#ifndef TASK_PRIMITIVES
#define TASK_PRIMITIVES

#include <mach/mach.h>

uint32_t task_allocate(mach_port_t task, uint32_t size);
kern_return_t task_deallocate(mach_port_t task, uint32_t address, size_t size);
kern_return_t task_protect(mach_port_t task, uint32_t address, uint32_t size, vm_prot_t protectionFlags);
kern_return_t task_write(mach_port_t task, uint32_t address, void *buffer, uint32_t size); 

#endif // TASK_PRIMITIVES
