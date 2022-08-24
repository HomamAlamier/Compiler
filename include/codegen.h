#ifndef CODEGEN_H
#define CODEGEN_H

#include <include/strings.h>
#include <include/list.h>

enum codegen_oper_type {
    CODEGEN_OPER_MOV = 1,
    CODEGEN_OPER_ADD,
    CODEGEN_OPER_SUB,
    CODEGEN_OPER_MUL,
    CODEGEN_OPER_DIV,
};

enum codegen_section_type {
    CODEGEN_SEC_TEXT = 1,
    CODEGEN_SEC_DATA,
    CODEGEN_SEC_BSS,
};

enum codegen_gen_type {
    CODEGEN_GEN_LOAD_FROM_STACK = 1,
    // (string_t*)
    CODEGEN_GEN_STACK_PUSH_STR,
    CODEGEN_GEN_STACK_PUSH_INT,
    CODEGEN_GEN_STACK_PUSH_REG,
    CODEGEN_GEN_STACK_PUSH_STACK,
    CODEGEN_GEN_STACK_POP_REG,
    CODEGEN_GEN_OPER_REG_TO_REG,
    CODEGEN_GEN_OPER_STR_TO_REG,
    CODEGEN_GEN_OPER_INT_TO_REG,
    CODEGEN_GEN_OPER_REG_TO_STACK,
    CODEGEN_GEN_OPER_STACK_TO_REG,
    CODEGEN_GEN_FUNC_RET,
    CODEGEN_GEN_FUNC_RET_CLEAN,
    CODEGEN_GEN_FUNC_SYMBOL,
    CODEGEN_GEN_FUNC_CALL,
    CODEGEN_GEN_DATA,
    CODEGEN_GEN_SECTION,
    CODEGEN_GEN_FILE_HEADER
};

enum codegen_reg_type {
    CODEGEN_REG_A = 1,
    CODEGEN_REG_B,
    CODEGEN_REG_C,
    CODEGEN_REG_D,
    CODEGEN_REG_STACK_POINTER,
    CODEGEN_REG_BASE_POINTER,
};

enum codegen_size_type {
    CODEGEN_SIZE_BYTE = 1,
    CODEGEN_SIZE_WORD = 2,
    CODEGEN_SIZE_DWORD = 4,
    CODEGEN_SIZE_QWORD = 8,
    CODEGEN_SIZE_TEN_BYTES = 10
};

enum codegen_types {
    CODEGEN_TYPE_INT = 1
};

typedef string_t* (*codegen_gen_code_func_t)(int, ...);
typedef size_t (*codegen_get_type_size_func_t)(int);

typedef struct codegen {
    string_t* name;
    string_t* assembler_name;
    codegen_gen_code_func_t gen_func;
    codegen_get_type_size_func_t size_func;
    int address_size;
    int bits;
} codegen_t;

#endif // CODEGEN_H
