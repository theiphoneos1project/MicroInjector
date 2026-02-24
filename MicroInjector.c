#include "MicroInjector.h"
#include "MicroInjectorInternal.h"
#include <objc/runtime.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

MicroInjectorReturn_t HookMessageEx(const Class cls, const SEL selector, IMP implementation, IMP *original) {
    if (cls == NULL || selector == NULL || implementation == NULL) {
        return MICROINJECTOR_PRECONDITION_FAILURE;
    }

    Method const method = class_getInstanceMethod(cls, selector);
    if (method == NULL) {
        return MICROINJECTOR_METHOD_NOT_FOUND;
    }

    if (original == NULL) {
        method_setImplementation(method, implementation);
    } else {
        IMP const oldImplementation = method_getImplementation(method);
        const char *const typeEncoding = method_getTypeEncoding(method);

        if (class_addMethod(cls, selector, implementation, typeEncoding)) {
            *original = oldImplementation;
        } else {
            *original = method_setImplementation(method, implementation);
        }
    }

    return MICROINJECTOR_SUCCESS;
}

IMP HookMessage(const Class cls, const SEL selector, IMP implementation, const char *prefix) {
    if (cls == NULL || selector == NULL || implementation == NULL) {
        return NULL;
    }

    Method const method = class_getInstanceMethod(cls, selector);
    if (method == NULL) {
        return NULL;
    }

    if (prefix != NULL) {
        const char *const name = sel_getName(selector);
        const char *const typeEncoding = method_getTypeEncoding(method);
        
        IMP const original = method_getImplementation(method);

        const size_t prefixLength = strlen(prefix);
        const size_t nameLength = strlen(name);

        char renamedName[prefixLength + nameLength + 1];
        bcopy(prefix, renamedName, prefixLength);
        bcopy(name, renamedName + prefixLength, nameLength + 1);

        class_addMethod(cls, sel_registerName(renamedName), original, typeEncoding);
        method_setImplementation(method, implementation);
        return NULL;
    }

    IMP original = NULL;
    HookMessageEx(cls, selector, implementation, &original);
    return original;
}

MicroInjectorReturn_t HookMemory(void *const target, const void *const data, const size_t size) {
    if (target == NULL || data == NULL || size == 0) {
        return MICROINJECTOR_PRECONDITION_FAILURE;
    }

    kern_return_t kr;
    
    const vm_address_t targetPage = VM_PAGE_OF(target);
    const vm_address_t offset = (vm_address_t)target - targetPage;

    // Without vm_region, dyld may race with the remap and win
    vm_address_t addr = targetPage;
    vm_size_t regionSize = 0;
    vm_region_basic_info_data_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT;
    mach_port_t object;
    vm_region(mach_task_self(), &addr, &regionSize, VM_REGION_BASIC_INFO, (vm_region_info_t)&info, &count, &object);

    kr = vm_protect(mach_task_self(), targetPage, ARM_PAGE_SIZE, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);

    if (kr == KERN_SUCCESS) {
        *(volatile uint32_t *)targetPage = *(volatile uint32_t *)targetPage;
    }

    vm_address_t newPage = 0;

    kr = vm_allocate(mach_task_self(), &newPage, ARM_PAGE_SIZE, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        return MICROINJECTOR_MEMORY_HOOK_ALLOCATION_FAILURE;
    }

    bcopy((void *)targetPage, (void *)newPage, ARM_PAGE_SIZE);
    bcopy(data, (void *)(newPage + offset), size);

    vm_prot_t currentProtection = VM_PROT_NONE;
    vm_prot_t maxProtection = VM_PROT_NONE;
    vm_address_t remapAddress = targetPage;

    vm_deallocate(mach_task_self(), targetPage, ARM_PAGE_SIZE);
    kr = vm_remap(mach_task_self(), &remapAddress, ARM_PAGE_SIZE, 0, VM_FLAGS_FIXED, mach_task_self(), newPage, TRUE, &currentProtection, &maxProtection, VM_INHERIT_COPY);
    if (kr != KERN_SUCCESS) {
        vm_deallocate(mach_task_self(), newPage, ARM_PAGE_SIZE);
        return MICROINJECTOR_MEMORY_HOOK_REMAP_FAILURE;
    }

    vm_deallocate(mach_task_self(), newPage, ARM_PAGE_SIZE);

    __clear_cache(target, (char *)target + size);
    return MICROINJECTOR_SUCCESS;
}

LoadedImageReference GetImageByName(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    uint32_t const count = _dyld_image_count();
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(_dyld_get_image_name(i), name) == 0) {
            return (LoadedImageReference)_dyld_get_image_header(i);
        }
    }

    return NULL;
}

void *FindSymbol(LoadedImageReference handle, const char *name) {
    if (name == NULL) {
        return NULL;
    }

    uint32_t const imageCount = _dyld_image_count();
    for (uint32_t i = 0; i < imageCount; i++) {
        const struct mach_header *const header = _dyld_get_image_header(i);
        if (handle != NULL && header != (struct mach_header *)handle) {
            continue;
        }

        intptr_t const slide = _dyld_get_image_vmaddr_slide(i);
        
        const struct symtab_command *symtab = NULL;
        const struct segment_command *linkedit = NULL;
        
        const struct load_command *lc = (const struct load_command *)(header + 1);
        for (uint32_t j = 0; j < header->ncmds; j++) {
            if (lc->cmd == LC_SYMTAB) {
                symtab = (const struct symtab_command *)lc;
            } else if (lc->cmd == LC_SEGMENT) {
                const struct segment_command *seg = (const struct segment_command *)lc;
                if (strcmp(seg->segname, "__LINKEDIT") == 0) {
                    linkedit = seg;
                }
            }

            lc = (const struct load_command *)((uintptr_t)lc + lc->cmdsize);
        }
        
        if (symtab == NULL || linkedit == NULL) {
            continue;
        }
        
        uintptr_t const linkeditBase = (uintptr_t)slide + linkedit->vmaddr - linkedit->fileoff;
        const struct nlist *const symbols = (const struct nlist *)(linkeditBase + symtab->symoff);
        const char *const strtab = (const char *)(linkeditBase + symtab->stroff);

        for (uint32_t j = 0; j < symtab->nsyms; j++) {
            if (symbols[j].n_value == 0) {
                continue;
            }
            
            const char *const symbolName = strtab + symbols[j].n_un.n_strx;

            if (strcmp(symbolName, name) == 0) {
                return (void *)(symbols[j].n_value + slide);
            }
        }

        if (handle != NULL) {
            break;
        }
    }

    return NULL;
}
