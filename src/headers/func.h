
#ifndef _H_FUNC
#define _H_FUNC

#include "typedefs.h"

Func* func_make(Allocator* alc, Unit* u, Scope* parent, char* name, char* export_name);
FuncArg* func_arg_make(Allocator* alc, Type* type);
void parse_handle_func_args(Parser* p, Func* func);
int func_get_reserve_count(Func* func);

struct Func {
    char* name;
    char* export_name;
    Build *b;
    Unit* unit;
    Scope* scope;
    Scope* scope_stack_reduce;
    //
    Chunk* chunk_args;
    Chunk* chunk_rett;
    Chunk* chunk_body;
    Chunk* body_end;
    //
    Map* args;
    Array* arg_types;
    Array* arg_values;
    Type* rett;
    Class* class;
    Array* cached_values;
    Map* errors;
    //
    bool is_static;
    bool is_inline;
    bool can_error;
    bool types_parsed;
    bool in_header;
};
struct FuncArg {
    Type* type;
    Value* value;
    Chunk* chunk_value;
    Decl* decl;
};
struct FuncError {
    ErrorCollection* collection;
    int value;
};
struct ErrorCollection {
    Map* errors;
};

#endif
