
#ifndef _H_FUNC
#define _H_FUNC

#include "typedefs.h"

Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name);
FuncArg* func_arg_make(Allocator* alc, Type* type);
void parse_handle_func_args(Fc* fc, Func* func);
int func_get_reserve_count(Func* func);

struct Func {
    char* name;
    char* export_name;
    Build *b;
    Fc* fc;
    Scope* scope;
    Scope* scope_gc_pop;
    //
    Chunk* chunk_args;
    Chunk* chunk_rett;
    Chunk* chunk_body;
    //
    Map* args;
    Array* arg_types;
    Type* rett;
    Class* class;
    Array* cached_values;
    //
    bool is_static;
    bool is_inline;
};
struct FuncArg {
    Type* type;
    Value* value;
    Chunk* chunk_value;
    Decl* decl;
};

#endif
