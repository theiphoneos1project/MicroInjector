#ifndef MICROINJECTOR_INTERNAL_H
#define MICROINJECTOR_INTERNAL_H

// Big thanks to staturnz

#include <stdint.h>
#include <stdbool.h>
#include <mach/mach.h>

#define ARM_PAGE_SIZE               0x1000

#define THUMB_ALIGN(addr)           ((void *)(((uint32_t)addr) & ~0x1))
#define SIZE_ALIGN(size)            (((uint32_t)(size) + (4 - 1)) & ~(4 - 1))
#define VM_PAGE_OF(addr)            ((uintptr_t)addr & ~((uintptr_t)(0xfff)))

// T16 pc relative instructions
#define T16_B_MASK                  0xF800
#define T16_B_OPCODE                0xE000
#define T16_B_COND_MASK             0xF000
#define T16_B_COND_OPCODE           0xD000
#define T16_LDR_LITERAL_MASK        0xF800
#define T16_LDR_LITERAL_OPCODE      0x4800
#define T16_HI_BLX_IMM_MASK         0xF800
#define T16_HI_BLX_IMM_OPCODE       0xF000
#define T16_LO_BLX_IMM_MASK         0xF001
#define T16_LO_BLX_IMM_OPCODE       0xE000

// A32 pc relative instructions
#define A32_B_MASK                  0x0F000000
#define A32_B_OPCODE                0x0A000000
#define A32_BL_MASK                 0x0F000000
#define A32_BL_OPCODE               0x0B000000
#define A32_BLX_IMM_MASK            0xFE000000
#define A32_BLX_IMM_OPCODE          0xFA000000
#define A32_ADR_ADD_MASK            0x0FEF0000
#define A32_ADR_ADD_OPCODE          0x028F0000
#define A32_ADR_SUB_MASK            0x0FEF0000
#define A32_ADR_SUB_OPCODE          0x024F0000
#define A32_LDR_LITERAL_MASK        0x0F3F0000
#define A32_LDR_LITERAL_OPCODE      0x051F0000
#define A32_VLDR_LITERAL_MASK       0x0F3F0F00
#define A32_VLDR_LITERAL_OPCODE     0x0D1F0B00
#define A32_VSTR_LITERAL_MASK       0x0F3F0F00
#define A32_VSTR_LITERAL_OPCODE     0x0D0F0B00

#define A32_LDR_PC_PC_N4            0xE51FF004
#define T16_LDR_R7_PC               0x4e00
#define T16_BX_R7                   0x4730     
#define T16_NOP                     0x46c0

typedef enum {
    MICROINJECTOR_ENCODING_UNKNOWN = -1,
    MICROINJECTOR_ENCODING_A32,
    MICROINJECTOR_ENCODING_T16
} MicroInjectorEncodingType_t;

typedef enum {
    MICROINJECTOR_HOOK_TYPE_NONE = -1,
    MICROINJECTOR_HOOK_TYPE_T16_DIRECT_BRANCH,
    MICROINJECTOR_HOOK_TYPE_A32_DIRECT_BRANCH,
    MICROINJECTOR_HOOK_TYPE_A32_LDR_PC,
    MICROINJECTOR_HOOK_TYPE_T16_LDR_BX,
    MICROINJECTOR_HOOK_TYPE_T16_NOP_LDR_BX
} MicroInjectorHookType_t;

typedef struct {
    MicroInjectorHookType_t type;
    uint32_t size;
    MicroInjectorEncodingType_t encoding;
} MicroInjectorHookInfo_t;

#endif /* MICROINJECTOR_INTERNAL_H */
