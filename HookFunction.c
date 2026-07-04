//
// Thanks to staturnz for the huge amount of help!
//

#include "MicroInjector.h"
#include "MicroInjectorInternal.h"

static bool t16_is_pc_relative(uint16_t inst) {
    if ((inst & T16_B_MASK) == T16_B_OPCODE) return true;
    if ((inst & T16_B_COND_MASK) == T16_B_COND_OPCODE) return true;
    if ((inst & T16_LDR_LITERAL_MASK) == T16_LDR_LITERAL_OPCODE) return true;
    return false;
}

static bool t16_multi_is_pc_relative(uint16_t inst_hi, uint16_t inst_lo) {
    if ((inst_hi & T16_HI_BLX_IMM_MASK) == T16_HI_BLX_IMM_OPCODE) {
        if ((inst_lo & T16_LO_BLX_IMM_MASK) == T16_LO_BLX_IMM_OPCODE) return true;
    }
    return false;
}

static bool a32_is_pc_relative(uint32_t inst) {
    if (inst == A32_LDR_PC_PC_N4) return false;
    if ((inst & A32_B_MASK) == A32_B_OPCODE) return true;
    if ((inst & A32_BL_MASK) == A32_BL_OPCODE) return true;
    if ((inst & A32_BLX_IMM_MASK) == A32_BLX_IMM_OPCODE) return true;
    if ((inst & A32_ADR_ADD_MASK) == A32_ADR_ADD_OPCODE) return true;
    if ((inst & A32_ADR_SUB_MASK) == A32_ADR_SUB_OPCODE) return true;
    if ((inst & A32_LDR_LITERAL_MASK) == A32_LDR_LITERAL_OPCODE) return true;
    if ((inst & A32_VLDR_LITERAL_MASK) == A32_VLDR_LITERAL_OPCODE) return true;
    if ((inst & A32_VSTR_LITERAL_MASK) == A32_VSTR_LITERAL_OPCODE) return true;
    return false;
}

static uint16_t t16_gen_b(int16_t imm) {
    uint16_t sign = (imm >> 16) & 0x1;
    imm = sign ? (imm + 0x4) : (imm - 0x4);
    return (0xe000 | ((imm >> 1) & 0x7ff));
}

static uint32_t a32_gen_b(int32_t imm) {
    uint16_t sign = (imm >> 31) & 0x1;
    imm = sign ? imm : (imm - 0x8);
    return ((0xe << 28) | (0x5 << 25) | ((imm >> 2) & 0xffffff));
}

static int32_t branch_difference(const void *const from, const void *const to, bool thumb) {
    if (!thumb) return (int32_t)to - (int32_t)(((uint32_t)from & ~0x3) + (((uint32_t)from > (uint32_t)to) ? 0x8 : 0x0));
    return (int32_t)to - (int32_t)(((uint32_t)from & ~0x1) + (((uint32_t)from > (uint32_t)to) ? 0x8 : 0x0));
}

static bool in_branch_range(const void *const start, const void *const end, bool thumb) {
    int32_t diff = branch_difference(start, end, thumb);
    if (thumb) return (diff >= -2048 && diff <= 2046);
    return (diff >= -(1 << 25) && diff <= ((1 << 25) - 4));
}

static int init_hook_info(const void *const target, const void *const replacement, bool original_needed, MicroInjectorHookInfo_t *info) {
    info->encoding = (THUMB_ALIGN(target) != target) ? MICROINJECTOR_ENCODING_T16 : MICROINJECTOR_ENCODING_A32;
    info->type = MICROINJECTOR_HOOK_TYPE_NONE;
    info->size = 0;

    uint8_t *const data = (uint8_t *)THUMB_ALIGN(target);

    for (uint32_t i = 0; i < 0x8; i += (info->encoding == MICROINJECTOR_ENCODING_A32) ? 0x4 : 0x2) {
        if (info->encoding == MICROINJECTOR_ENCODING_A32) {
            if (original_needed && a32_is_pc_relative(*(uint32_t *)(data + i))) {
                break;
            }

            info->size += 0x4;
        } else {
            const uint16_t inst_hi = *(uint16_t *)(data + i);
            const uint16_t inst_lo = *(uint16_t *)(data + i + 0x2);
            
            if (original_needed && t16_multi_is_pc_relative(inst_hi, inst_lo)) {
                break;
            }
    
            if (original_needed && t16_is_pc_relative(inst_hi)) {
                break;
            }

            info->size += 0x2;
        }
    }

    if (info->encoding == MICROINJECTOR_ENCODING_A32) {
        if (info->size >= 0x4 && in_branch_range(target, replacement, false)) {
            info->type = MICROINJECTOR_HOOK_TYPE_A32_DIRECT_BRANCH;
        } else if (info->size >= 0x8) {
            info->type = MICROINJECTOR_HOOK_TYPE_A32_LDR_PC;
        }
    } else {
        if (info->size >= 0x2 && in_branch_range(target, replacement, true)) {
            info->type = MICROINJECTOR_HOOK_TYPE_T16_DIRECT_BRANCH;
        } else if (info->size >= 0x8 && ((uint32_t)THUMB_ALIGN(target) % 4) == 0) {
            info->type = MICROINJECTOR_HOOK_TYPE_T16_LDR_BX;
        } else if (info->size >= 0xa && ((uint32_t)THUMB_ALIGN(target) % 2) == 0) {
            info->type = MICROINJECTOR_HOOK_TYPE_T16_NOP_LDR_BX;
        }
    }

    return (info->type != MICROINJECTOR_HOOK_TYPE_NONE) ? 0 : -1;
}

static void install_hook(void *const from, const void *const to, MicroInjectorHookType_t type) {
    switch (type) {
        case MICROINJECTOR_HOOK_TYPE_T16_DIRECT_BRANCH: {
            *(uint16_t *)THUMB_ALIGN(from) = t16_gen_b(branch_difference(from, to, true));
        } break;

        case MICROINJECTOR_HOOK_TYPE_A32_DIRECT_BRANCH: {
            *(uint32_t *)THUMB_ALIGN(from) = a32_gen_b(branch_difference(from, to, false));
        } break;
   
        case MICROINJECTOR_HOOK_TYPE_A32_LDR_PC: {
            uint32_t *data = (uint32_t *)THUMB_ALIGN(from);
            data[0] = A32_LDR_PC_PC_N4;
            data[1] = (uint32_t)to;
        } break;

        case MICROINJECTOR_HOOK_TYPE_T16_LDR_BX: {
            uint16_t *data = (uint16_t *)THUMB_ALIGN(from);
            data[0] = T16_LDR_R7_PC;
            data[1] = T16_BX_R7;
            *(uint32_t *)(data + 2) = (uint32_t)to;
        } break;

        case MICROINJECTOR_HOOK_TYPE_T16_NOP_LDR_BX: {
            uint16_t *data = (uint16_t *)THUMB_ALIGN(from);
            data[0] = T16_NOP;
            data[1] = T16_LDR_R7_PC;
            data[2] = T16_BX_R7;
            *(volatile uint32_t *)(((uint32_t)THUMB_ALIGN(from)) + 0x6) = (uint32_t)to;
        } break;

        case MICROINJECTOR_HOOK_TYPE_NONE:
        default: break;
    }
}

static MicroInjectorHookType_t indirect_hook_type(MicroInjectorEncodingType_t encoding, const void *const base) {
    if (encoding == MICROINJECTOR_ENCODING_A32) {
        return MICROINJECTOR_HOOK_TYPE_A32_LDR_PC;
    }
    if (((uint32_t)THUMB_ALIGN(base) % 4) == 0) {
        return MICROINJECTOR_HOOK_TYPE_T16_LDR_BX;
    }

    return MICROINJECTOR_HOOK_TYPE_T16_NOP_LDR_BX;
}

static MicroInjectorEncodingType_t encoding_for_target(const void *const target) {
    return THUMB_ALIGN(target) != target ? MICROINJECTOR_ENCODING_T16 : MICROINJECTOR_ENCODING_A32;
}

static MicroInjectorReturn_t build_trampoline(void *const target, const MicroInjectorHookInfo_t *const info, void **original) {
    const uint32_t trampolineSize = SIZE_ALIGN(info->size + 0x10);
    
    kern_return_t kr;
    
    vm_address_t trampolineBase = 0;

    kr = vm_allocate(mach_task_self(), &trampolineBase, trampolineSize, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        return MICROINJECTOR_FUNCTION_HOOK_TRAMPOLINE_ALLOCATION_FAILURE;
    }

    bcopy(THUMB_ALIGN(target), (void *)trampolineBase, info->size);

    void *const trampolineFrom = (void *)(trampolineBase + info->size);
    const void *const trampolineTo = (void *)((uint32_t)THUMB_ALIGN(target) + info->size);
    install_hook(trampolineFrom, trampolineTo, indirect_hook_type(info->encoding, trampolineFrom));

    kr = vm_protect(mach_task_self(), trampolineBase, trampolineSize, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        vm_deallocate(mach_task_self(), trampolineBase, trampolineSize);
        return MICROINJECTOR_FUNCTION_HOOK_TRAMPOLINE_PROTECT_FAILURE;
    }
    
    __clear_cache((void *)trampolineBase, (void *)(trampolineBase + trampolineSize));

    if (info->encoding == MICROINJECTOR_ENCODING_A32) {
        *original = (void *)trampolineBase;
    } else {
        *original = (void *)(trampolineBase | 0x1);
    }

    return MICROINJECTOR_SUCCESS;
}

static MicroInjectorReturn_t remap_page_with_hook(void *const target, const void *const replacement, MicroInjectorHookType_t hookType) {
    const vm_address_t targetPage = (vm_address_t)VM_PAGE_OF(target);
    const vm_address_t offset = (vm_address_t)THUMB_ALIGN(target) & 0xFFF;

    kern_return_t kr;

    vm_address_t newPage = 0;

    kr = vm_allocate(mach_task_self(), &newPage, ARM_PAGE_SIZE, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        return MICROINJECTOR_FUNCTION_HOOK_PAGE_ALLOCATION_FAILURE;
    }

    bcopy((void *)targetPage, (void *)newPage, ARM_PAGE_SIZE);
    install_hook((void *)(newPage + offset), replacement, hookType);

    vm_prot_t currentProtection = VM_PROT_NONE;
    vm_prot_t maxProtection = VM_PROT_NONE;

    vm_address_t remapAddress = targetPage;

    vm_deallocate(mach_task_self(), targetPage, ARM_PAGE_SIZE);
    kr = vm_remap(mach_task_self(), &remapAddress, ARM_PAGE_SIZE, 0, VM_FLAGS_FIXED, mach_task_self(), newPage, TRUE, &currentProtection, &maxProtection, VM_INHERIT_COPY);
    if (kr != KERN_SUCCESS) {
        vm_deallocate(mach_task_self(), newPage, ARM_PAGE_SIZE);
        return MICROINJECTOR_FUNCTION_HOOK_REMAP_FAILURE;
    }

    __clear_cache(THUMB_ALIGN(target), (void *)((uint32_t)THUMB_ALIGN(target) + ARM_PAGE_SIZE));
    return MICROINJECTOR_SUCCESS;
}

static bool read_existing_hook(void *const target, MicroInjectorEncodingType_t encoding, void **previous) {
    if (encoding == MICROINJECTOR_ENCODING_A32) {
        uint32_t *const instruction = (uint32_t *)THUMB_ALIGN(target);
        if (instruction[0] == A32_LDR_PC_PC_N4) {
            *previous = (void *)instruction[1];
            return true;
        }
    } else {
        uint16_t *const instruction = (uint16_t *)THUMB_ALIGN(target);
        
        if (instruction[0] == T16_LDR_R7_PC && instruction[1] == T16_BX_R7) {
            *previous = (void *)*(uint32_t *)(instruction + 2);
            return true;
        }

        if (instruction[0] == T16_NOP && instruction[1] == T16_LDR_R7_PC && instruction[2] == T16_BX_R7) {
            *previous = (void *)*(uint32_t *)((uint32_t)THUMB_ALIGN(target) + 0x6);
            return true;
        }
    }

    return false;
}

MicroInjectorReturn_t HookFunction(void *const target, void *const replacement, void **original) {
    if (target == NULL || replacement == NULL) {
        return MICROINJECTOR_PRECONDITION_FAILURE;
    }

    const MicroInjectorEncodingType_t encoding = encoding_for_target(target);

    void *previous = NULL;
    if (read_existing_hook(target, encoding, &previous)) {
        if (original != NULL) {
            *original = previous;
        }

        return remap_page_with_hook(target, replacement, indirect_hook_type(encoding, target));
    }

    MicroInjectorHookInfo_t info = {0};
    if (init_hook_info(target, replacement, (original != NULL), &info) != 0) {
        return MICROINJECTOR_FUNCTION_HOOK_NO_VALID_TYPE;
    }

    if (original != NULL) {
        MicroInjectorReturn_t trampolineStatus = build_trampoline(target, &info, original);
        
        if (trampolineStatus != MICROINJECTOR_SUCCESS) {
            return trampolineStatus;
        }
    }

    MicroInjectorReturn_t remapStatus = remap_page_with_hook(target, replacement, info.type);
    if (remapStatus != MICROINJECTOR_SUCCESS && original != NULL && *original != NULL) {
        vm_deallocate(mach_task_self(), (vm_address_t)THUMB_ALIGN(*original), SIZE_ALIGN(info.size + 0x10));
        *original = NULL;
    }

    return remapStatus;
}
