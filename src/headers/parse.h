
#ifndef _H_PARSE
#define _H_PARSE

#include "typedefs.h"

// Scopes
Scope* scope_make(Allocator* alc, int type, Scope* parent);
void scope_set_idf(Scope* scope, char*name, Idf* idf, Fc* fc);
void scope_add_decl(Scope* scope, Decl* decl);
// Read
char* tok(Fc* fc, bool allow_space, bool allow_newline, bool update);
void tok_back(Fc* fc);
void tok_expect(Fc* fc, char* expect, bool allow_space, bool allow_newline);
char tok_id_next(Fc* fc);
char tok_id_next_ignore_spacing(Fc* fc);
char tok_read_byte(Fc* fc, int offset);
void tok_skip_whitespace(Fc* fc);
bool tok_next_is_whitespace(Fc* fc);
char* chunk_tok(Chunk* chunk, bool allow_space, bool allow_newline, bool read_only);
char* chunk_read(Chunk* chunk, int *i_ref);

struct Scope {
    Scope* parent;
    Map* identifiers;
    Array* ast;
    Type* rett;
    Array* decls;
    int type;
    bool must_return;
    bool did_return;
};
struct Idf {
    int type;
    void* item;
};
struct Decl {
    Type* type;
    char *ir_var;
    bool is_mut;
    bool is_arg;
};

#endif
