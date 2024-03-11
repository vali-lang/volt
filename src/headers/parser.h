
#ifndef _H_PARSER
#define _H_PARSER

#include "typedefs.h"

Parser* parser_make(Allocator* alc, Build* b);
void parser_set_chunk(Parser* p, Chunk* chunk, bool sub_chunk);
void parser_pop_chunk(Parser* p);
Value *read_value_from_other_chunk(Parser *p, Allocator* alc, Chunk *chunk, Scope *idf_scope);

struct Parser {
    Build* b;
    // State
    Unit* unit;
    Chunk* chunks;
    Chunk* chunk;
    Chunk* scope_end;
    //
    Func* func;
    Class* class;
    //
    Scope* scope;
    Scope* loop_scope;
    //
    char* tkn;
    //
    int line;
    int col;
    int scope_end_i;
    //
    int chunk_index;
    bool in_header;
};

#endif