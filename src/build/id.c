
#include "../all.h"

Idf* idf_make(Allocator* alc, int type, void* item) {
    Idf* idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = item;
    return idf;
}
Decl* decl_make(Allocator* alc, Type* type, bool is_arg) {
    Decl* d = al(alc, sizeof(Decl));
    d->type = type;
    d->ir_var = NULL;
    d->is_arg = is_arg;
    d->is_mut = false;
    return d;
}

char* gen_export_name(Nsc* nsc, char* suffix) {
    char name[512];
    Pkc* pkc = nsc->pkc;
    Build* b = pkc->b;
    sprintf(name, "%s__%s__%s_%d", pkc->name, nsc->name, suffix, b->export_count++);
    return dups(b->alc, name);
}


Idf* read_idf(Fc* fc, Scope* scope, char* first_part, bool must_exist) {
    //
    char* nsc = NULL;
    char* name = first_part;
    Build* b = fc->b;

    if(tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == ':') {
        nsc = name;
        tok(fc, false, false, true);
        name = tok(fc, false, false, true);
        if(!is_valid_varname(name)) {
            sprintf(b->char_buf, "Missing string closing tag '\"', compiler reached end of file");
            parse_err(fc->chunk_parse, b->char_buf);
        }
    }

    if(nsc) {
        Idf* idf = scope_find_idf(fc->scope, nsc, false);
        if(!idf || idf->type != idf_scope) {
            sprintf(b->char_buf, "Unknown namespace: '%s' (You can import namespaces using the 'use' keyword)", nsc);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        scope = idf->item;
    }

    Idf* idf = scope_find_idf(scope, name, true);
    if(!idf && !nsc) {
        if(str_is(name, "string") || str_is(name, "ptr") || str_is(name, "bool") || str_is(name, "int") || str_is(name, "uint") || str_is(name, "i32") || str_is(name, "u8")) {
            Nsc* ns = get_volt_nsc(fc->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        }
        if(str_is(name, "print") || str_is(name, "println")) {
            Nsc* ns = get_volt_nsc(fc->b, "io");
            idf = scope_find_idf(ns->scope, name, true);
        }
    }

    if(!idf && must_exist) {
        if(nsc) sprintf(b->char_buf, "Unknown identifier: '%s:%s'", nsc, name);
        else sprintf(b->char_buf, "Unknown identifier: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    return idf;
}

Idf* scope_find_idf(Scope* scope, char* name, bool recursive) {
    while(scope) {
        Idf* idf = map_get(scope->identifiers, name);
        if(!idf) {
            if(!recursive)
                break;
            scope = scope->parent;
            continue;
        }
        return idf;
    }
    return NULL;
}

Func *get_volt_func(Build *b, char *namespace, char *name) {
    Nsc* nsc = get_volt_nsc(b, namespace);
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(idf && idf->type == idf_func) {
        return idf->item;
    }
    printf("VOLT FUNCTION NOT FOUND: '%s'", name);
    exit(1);
}
