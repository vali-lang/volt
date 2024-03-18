
#ifndef _H_UNIT
#define _H_UNIT

#include "typedefs.h"

void unit_validate(Unit *u, Parser *p);
void validate_class(Parser *p, Class* class);

struct Unit {
    Build *b;
    Nsc *nsc;
    //
    char *path_o;
    char *path_ir;
    char *path_cache;
    char *hash;
    //
    Array *funcs;
    Array *classes;
    Array *aliasses;
    Array *globals;
    //
    int id;
    int string_count;
    //
    bool ir_changed;
    bool contains_main_func;
};

#endif