
#include "../all.h"

void stage_types(Fc *fc);

void stage_2_types(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 2 | Scan types: %s\n", fc->path);

    stage_types(fc);

    stage_add_item(b->stage_3_values, fc);
}

void stage_types(Fc *fc) {
    Array* funcs = fc->funcs;
    for(int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        stage_types_func(fc, func);
    }
}

void stage_types_func(Fc* fc, Func* func) {

    Build *b = fc->b;

    if(func->class && !func->is_static) {
        FuncArg *arg = func_arg_make(b->alc, type_gen_class(b->alc, func->class));
        map_set(func->args, "this", arg);
        Decl *decl = decl_make(b->alc, arg->type, true);
        Idf *idf = idf_make(b->alc, idf_decl, decl);
        scope_set_idf(func->scope, "this", idf, fc);
    }

    if(func->chunk_args) {
        *fc->chunk_parse = *func->chunk_args;
        char* tkn = tok(fc, true, true, true);
        while(!str_is(tkn, ")")) {
            char* name = tkn;
            if(!is_valid_varname(name)) {
                sprintf(b->char_buf, "Invalid function argument name: '%s'", name);
                parse_err(fc->chunk_parse, b->char_buf);
            }

            tok_expect(fc, ":", true, false);
            Type* type = read_type(fc, fc->alc, fc->scope, false);
            if(type->type == type_void) {
                sprintf(b->char_buf, "You cannot use void types for arguments");
                parse_err(fc->chunk_parse, b->char_buf);
            }

            FuncArg* arg = func_arg_make(b->alc, type);
            map_set(func->args, name, arg);

            Decl* decl = decl_make(b->alc, type, true);
            Idf* idf = idf_make(b->alc, idf_decl, decl);
            scope_set_idf(func->scope, name, idf, fc);

            tkn = tok(fc, true, true, true);
            if(str_is(tkn, "=")) {
                arg->chunk_value = chunk_clone(b->alc, fc->chunk_parse);
                skip_value(fc);
                tkn = tok(fc, true, true, true);
            }

            if(str_is(tkn, ",")) {
                tkn = tok(fc, true, true, true);
                continue;
            }
            if(!str_is(tkn, ")")) {
                sprintf(b->char_buf, "Unexpected token: '%s', expected ')'", tkn);
                parse_err(fc->chunk_parse, b->char_buf);
            }
        }
    }
    if(func->chunk_rett) {
        *fc->chunk_parse = *func->chunk_rett;
        Type *type = read_type(fc, fc->alc, fc->scope, false);
        func->rett = type;
    } else {
        func->rett = type_gen_void(fc->alc);
    }
}
