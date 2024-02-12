
#include "../all.h"

Type* type_make(Allocator* alc, int type) {
    Type* t = al(alc, sizeof(Type));
    t->type = type;
    t->size = 0;
    t->class = NULL;
    t->func_args = NULL;
    t->func_rett = NULL;
    t->is_pointer = false;
    t->is_signed = false;
    t->nullable = false;
    return t;
}

Type* read_type(Fc* fc, Allocator* alc, Scope* scope, bool allow_newline) {
    //
    Build* b = fc->b;

    Type *type = NULL;
    bool nullable = false;
    bool is_inline = false;

    char* tkn = tok(fc, true, allow_newline, true);
    if(str_is(tkn, "?")) {
        nullable = true;
        tkn = tok(fc, false, false, true);
    }

    int t = fc->chunk_parse->token;
    if(t == tok_id) {
        if (!is_inline && str_is(tkn, "void")) {
            return type_make(alc, type_void);
        }
        if (str_is(tkn, "fn")) {
            tok_expect(fc, "(", false, false);
            Array* args = array_make(alc, 4);
            tok_skip_space(fc);
            while (tok_id_next(fc) != tok_scope_close) {
                Type* type = read_type(fc, alc, scope, false);
                array_push(args, type);
                tok_skip_space(fc);
                if (tok_id_next(fc) != tok_scope_close) {
                    tok_expect(fc, ",", true, false);
                    tok_skip_space(fc);
                }
            }
            tok_expect(fc, ")", false, false);
            tok_expect(fc, "(", false, false);
            tok_skip_space(fc);
            Type* rett;
            if (tok_id_next(fc) != tok_scope_close) {
                rett = read_type(fc, alc, scope, false);
            } else {
                rett = type_gen_void(alc);
            }
            tok_expect(fc, ")", false, false);
            //
            Type *t = type_make(alc, type_func);
            t->func_rett = rett;
            t->func_args = args;
            t->size = b->ptr_size;
            t->is_pointer = true;
            return t;
        }

        if (str_is(tkn, "struct")) {
            is_inline = true;
            tkn = tok(fc, true, false, true);
            t = fc->chunk_parse->token;
        }

        if (t == tok_id) {
            // Identifier
            Id id;
            read_id(fc, tkn, &id);
            Idf *idf = idf_by_id(fc, scope, &id, false);
            if (!idf) {
                id.ns ? sprintf(b->char_buf, "Unknown type: '%s:%s'", id.ns, id.name)
                      : sprintf(b->char_buf, "Unknown type: '%s'", id.name);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            if (idf->type == idf_class) {
                Class *class = idf->item;
                type = type_gen_class(alc, class);
                if (is_inline)
                    type->is_pointer = false;
            }
        }
    }

    if (type) {
        if(nullable) {
            if (!type->is_pointer) {
                char buf[256];
                sprintf(b->char_buf, "This type cannot be null: '?%s'", type_to_str(type, buf));
                parse_err(fc->chunk_parse, b->char_buf);
            }
            type->nullable = true;
        }
        return type;
    }

    sprintf(b->char_buf, "Invalid type: '%s'", tkn);
    parse_err(fc->chunk_parse, b->char_buf);
    return NULL;
}

Type* type_gen_void(Allocator* alc) {
    return type_make(alc, type_void);
}
Type* type_gen_null(Allocator* alc, Build* b) {
    Type* t = type_make(alc, type_null);
    t->is_pointer = true;
    t->size = b->ptr_size;
    return t;
}
Type* type_gen_class(Allocator* alc, Class* class) {
    Type* t = type_make(alc, type_struct);
    t->class = class;
    t->size = class->b->ptr_size;

    const int ct = class->type;
    if(ct == ct_bool) {
        t->type = type_bool;
    } else if (ct == ct_ptr) {
        t->type = type_ptr;
    } else if (ct == ct_int) {
        t->type = type_int;
        t->size = class->size;
        t->is_signed = class->is_signed;
    } else if (ct == ct_float) {
        t->type = type_float;
    }

    if(ct == ct_class || ct == ct_struct || ct == ct_ptr)
        t->is_pointer = true;

    return t;
}
Type* type_gen_func(Allocator* alc, Func* func) {
    Type* t = type_make(alc, type_func);
    t->func_rett = func->rett;
    t->func_args = func->arg_types;
    t->size = func->b->ptr_size;
    t->is_pointer = true;
    return t;
}
Type* type_gen_volt(Allocator* alc, Build* b, char* name) {
    Nsc* nsc = get_volt_nsc(b, "type");
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(idf && idf->type == idf_class) {
        return type_gen_class(alc, idf->item);
    }
    printf("VOLT TYPE NOT FOUND: '%s'", name);
    exit(1);
}

Type* type_gen_number(Allocator* alc, Build* b, int size, bool is_float, bool is_signed) {
    if(is_float) {
        if(size == 4)
            return type_gen_volt(alc, b, "f32");
        if(size == 8)
            return type_gen_volt(alc, b, "f64");
    } else {
        if(size == 1)
            return type_gen_volt(alc, b, is_signed ? "i8" : "u8");
        if(size == 2)
            return type_gen_volt(alc, b, is_signed ? "i16" : "u16");
        if(size == 4)
            return type_gen_volt(alc, b, size == b->ptr_size ? (is_signed ? "int" : "uint") : (is_signed ? "i32" : "u32"));
        if(size == 8)
            return type_gen_volt(alc, b, size == b->ptr_size ? (is_signed ? "int" : "uint") : (is_signed ? "i64" : "u64"));
    }
    return NULL;
}

bool type_compat(Type* t1, Type* t2, char** reason) {
    if (t2->type == type_null && (t1->type == type_ptr || (t1->type == type_struct && t1->nullable))) {
        return true;
    }
    if (t1->type != t2->type) {
        *reason = "different kind of types";
        return false;
    }
    if (t1->is_signed != t2->is_signed) {
        *reason = "signed vs unsigned";
        return false;
    }
    if (t1->size != t2->size) {
        *reason = "types have different sizes";
        return false;
    }
    if (t1->is_pointer != t2->is_pointer) {
        *reason = "pointer vs no-pointer";
        return false;
    }
    if (t1->type == type_struct && t1->class != t2->class) {
        *reason = "different classes";
        return false;
    }
    if (t2->nullable && !t1->nullable) {
        *reason = "non-null vs null-able-type";
        return false;
    }
    return true;
}
void type_check(Chunk* chunk, Type* t1, Type* t2) {
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)) {
        Build* b = chunk->b;
        char t1b[256];
        char t2b[256];
        sprintf(b->char_buf, "Types are not compatible | %s <-> %s | Reason: %s", type_to_str(t1, t1b), type_to_str(t2, t2b), reason ? reason : "?");
        parse_err(chunk, b->char_buf);
    }
}

bool type_is_void(Type* type) { return type->type == type_void; }
bool type_is_bool(Type* type) { return type->type == type_bool; }
bool type_is_gc(Type* type) { return type->is_pointer && type->type == type_struct && type->class->type == ct_class && false; }

char* type_to_str(Type* t, char* res) {

    strcpy(res, "");
    if (t->nullable) {
        strcat(res, "?");
    }

    if (t->type == type_void) {
        strcpy(res, "void");
    } else if (t->type == type_null) {
        strcpy(res, "null");
    } else if (t->class) {
        Class *class = t->class;
        strcat(res, class->name);
    }

    return res;
}

