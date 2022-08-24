#include <include/x86_32_codegen.h>
#include <include/variable.h>
#include <include/lexer.h>
#include <stdlib.h>
#include <stdarg.h>

const char* reg_str(int reg) {
    switch (reg) {
    case CODEGEN_REG_A: return "eax";
    case CODEGEN_REG_B: return "ebx";
    case CODEGEN_REG_C: return "ecx";
    case CODEGEN_REG_D: return "edx";
    case CODEGEN_REG_STACK_POINTER: return "esp";
    case CODEGEN_REG_BASE_POINTER: return "ebp";
    }
    return "";
}

const char* oper_str(int oper) {
    switch (oper) {
    case CODEGEN_OPER_MOV: return "mov";
    case CODEGEN_OPER_ADD: return "add";
    case CODEGEN_OPER_SUB: return "sub";
    case CODEGEN_OPER_MUL: return "mul";
    case CODEGEN_OPER_DIV: return "div";
    }
    return "";
}

const char* sec_str(int sec) {
    switch (sec) {
    case CODEGEN_SEC_TEXT: return ".text";
    case CODEGEN_SEC_DATA: return ".data";
    case CODEGEN_SEC_BSS: return ".bss";
    }
    return "";
}

const char* data_size_str(int szt) {
    switch (szt) {
    case CODEGEN_SIZE_BYTE: return "db";
    case CODEGEN_SIZE_WORD: return "dw";
    case CODEGEN_SIZE_DWORD: return "dd";
    case CODEGEN_SIZE_QWORD: return "dq";
    case CODEGEN_SIZE_TEN_BYTES: return "dt";
    }
    return "";
}

const char* bss_size_str(int szt) {
    switch (szt) {
    case CODEGEN_SIZE_BYTE: return "resb";
    case CODEGEN_SIZE_WORD: return "resw";
    case CODEGEN_SIZE_DWORD: return "resd";
    case CODEGEN_SIZE_QWORD: return "resq";
    case CODEGEN_SIZE_TEN_BYTES: return "rest";
    }
    return "";
}

const char* size_str(int szt) {
    switch (szt) {
    case CODEGEN_SIZE_BYTE: return "byte";
    case CODEGEN_SIZE_WORD: return "word";
    case CODEGEN_SIZE_DWORD: return "dword";
    case CODEGEN_SIZE_QWORD: return "qword";
    case CODEGEN_SIZE_TEN_BYTES: return "tbyte";
    }
    return "";
}


string_t* x86_32_codegen_gen_func(int type, ...)  {
    va_list args;
    va_start(args, type);

    string_t* ret = string_empty();

    switch (type) {
    case CODEGEN_GEN_LOAD_FROM_STACK:
    {
        int reg = va_arg(args, int);
        ret = string_format("mov %s, [ebp + %d]", reg_str(reg), 8 + va_arg(args, int));
        break;
    }
    case CODEGEN_GEN_STACK_PUSH_STR:
        ret = string_format("push %S", va_arg(args, string_t*));
        break;
    case CODEGEN_GEN_STACK_PUSH_INT:
        ret = string_format("push %d", va_arg(args, int));
        break;
    case CODEGEN_GEN_STACK_PUSH_REG:
        ret = string_format("push %s", reg_str(va_arg(args, int)));
        break;
    case CODEGEN_GEN_STACK_PUSH_STACK:
        ret = string_format("push [ebp + %d]", 8 + va_arg(args, int));
        break;
    case CODEGEN_GEN_STACK_POP_REG:
        ret = string_format("pop %s", reg_str(va_arg(args, int)));
        break;
    case CODEGEN_GEN_OPER_REG_TO_REG:
    {
        int oper = va_arg(args, int);
        int reg1 = va_arg(args, int);
        ret = string_format("%s %s, %s", oper_str(oper), reg_str(va_arg(args, int)), reg_str(reg1));
        break;
    }
    case CODEGEN_GEN_OPER_STR_TO_REG:
    {
        int oper = va_arg(args, int);
        string_t* val = va_arg(args, string_t*);
        ret = string_format("%s %s, %S", oper_str(oper), reg_str(va_arg(args, int)), val);
        break;
    }
    case CODEGEN_GEN_OPER_INT_TO_REG:
    {
        int oper = va_arg(args, int);
        int val = va_arg(args, int);
        ret = string_format("%s %s, %d", oper_str(oper), reg_str(va_arg(args, int)), val);
        break;
    }
    case CODEGEN_GEN_OPER_REG_TO_STACK:
    {
        int oper = va_arg(args, int);
        int reg = va_arg(args, int);
        int offset = va_arg(args, int);
        ret = string_format("%s [ebp + %d], %s", oper_str(oper), offset, reg_str(reg));
        break;
    }
    case CODEGEN_GEN_OPER_STACK_TO_REG:
    {
        int oper = va_arg(args, int);
        int offset = va_arg(args, int);
        int reg = va_arg(args, int);
        ret = string_format("%s %s, [ebp + %d]", oper_str(oper), reg_str(reg), offset);
        break;
    }
    case CODEGEN_GEN_FUNC_RET:
        ret = init_string("ret");
        break;
    case CODEGEN_GEN_FUNC_RET_CLEAN:
        ret = string_format("ret %d", va_arg(args, int));
        break;
    case CODEGEN_GEN_FUNC_SYMBOL:
        ret = string_format("_%S:", va_arg(args, string_t*));
        break;
    case CODEGEN_GEN_FUNC_CALL:
        ret = string_format("call _%S", va_arg(args, string_t*));
        break;
    case CODEGEN_GEN_DATA:
    {
        int sz = va_arg(args, int);
        ret = string_format("%S %s %S", va_arg(args, string_t*), data_size_str(sz), va_arg(args, string_t*));
        break;
    }
    case CODEGEN_GEN_SECTION:
        ret = string_format("section %s", sec_str(va_arg(args, int)));
        break;
    case CODEGEN_GEN_FILE_HEADER:
    {
        codegen_t* gen = va_arg(args, codegen_t*);
        ret = string_format("[bits %d]\nglobal _start\n", gen->bits);
        break;
    }
    }

    va_end(args);
    return ret;
}

size_t x86_32_codegen_get_size(int type) {
    switch (type) {
    case CODEGEN_TYPE_INT: return 4;
    }
    return 0;
}

codegen_t* init_x86_32_codegen() {
    codegen_t* this = calloc(1, sizeof(struct codegen));
    this->name = init_string("x86_32_codegen");
    this->assembler_name = init_string("nasm");
    this->gen_func = x86_32_codegen_gen_func;
    this->size_func = x86_32_codegen_get_size;
    this->address_size = 4;
    this->bits = 32;
    return this;
}
