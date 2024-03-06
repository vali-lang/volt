
#include "../all.h"

Class* class_make(Allocator* alc, Build* b, int type) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->type = type;
    c->name = NULL;
    c->ir_name = NULL;
    c->body = NULL;
    c->scope = NULL;
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    //
    c->size = -1;
    c->gc_fields = 0;
    c->gc_vtable_index = 0;
    //
    c->packed = false;
    c->is_signed = true;
    //
    c->generics = NULL;
    c->generic_names = NULL;
    c->generic_types = NULL;
    c->is_generic_base = false;
    //
    return c;
}
ClassProp* class_prop_make(Build* b, Type* type, bool skip_default_value) {
    ClassProp* prop = al(b->alc, sizeof(ClassProp));
    prop->chunk_type = NULL;
    prop->chunk_value = NULL;
    prop->index = -1;
    prop->type = type;
    prop->skip_default_value = skip_default_value;
    return prop;
}

ClassProp* class_get_prop(Build* b, Class* class, char* name) {
    ClassProp* prop = map_get(class->props, name);
    if(!prop) {
        sprintf(b->char_buf, "Class property '%s' not found", name);
        build_err(b, b->char_buf);
    }
    return prop;
}

int class_determine_size(Build* b, Class* class) {

    int type = class->type;
    if(type != ct_class && type != ct_struct) {
        return class->size;
    }

    int size = 0;
    int largest = 0;
    int propc = class->props->values->length;
    for (int i = 0; i < propc; i++) {
        //
        ClassProp *prop = array_get_index(class->props->values, i);
        int prop_size = type_get_size(b, prop->type);
        if (prop_size == -1) {
            return -1;
        }
        if (prop_size > largest) {
            largest = prop_size;
        }
        size += prop_size;
        //
        if (class->packed)
            continue;
        // Calculate padding
        int next_i = i + 1;
        if (next_i < propc) {
            ClassProp *prop = array_get_index(class->props->values, i);
            int next_size = type_get_size(b, prop->type);
            if (next_size == -1) {
                return -1;
            }
            if (next_size > b->ptr_size)
                next_size = b->ptr_size;
            int rem = size % next_size;
            if (rem > 0) {
                // Add padding
                size += next_size - rem;
            }
        }
    }
    // if (!class->packed) {
    //     if (largest > b->ptr_size)
    //         largest = b->ptr_size;
    //     int rem = size % largest;
    //     size += rem;
    // }

    if (size == 0) {
        size = 8;
    }
    class->size = size;

    return size;
}

void class_generate_internals(Fc* fc, Build* b, Class* class) {

    Allocator *alc = b->alc;

    if (class->type == ct_class && map_get(class->funcs, "_v_transfer") == NULL) {
        char* buf = b->char_buf;
        // STACK
        // Idf *idf = idf_make(b->alc, idf_global, get_volt_global(b, "mem", "stack"));
        // scope_set_idf(class->scope, "STACK", idf, fc);

        // VTABLE_INDEX
        printf("Class: %s | vt: %d | size: %d\n", class->name, class->gc_vtable_index, class->size);
        //
        Idf* idf = idf_make(b->alc, idf_value, vgen_int(b->alc, class->gc_vtable_index, type_gen_number(b->alc, b, 4, false, false)));
        scope_set_idf(class->scope, "VTABLE_INDEX", idf, fc);
        //
        idf = idf_make(b->alc, idf_value, vgen_int(b->alc, class->size, type_gen_number(b->alc, b, b->ptr_size, false, false)));
        scope_set_idf(class->scope, "SIZE", idf, fc);
        //
        idf = idf_make(b->alc, idf_global, get_volt_global(b, "mem", "gc_transfer_size"));
        scope_set_idf(class->scope, "GC_TRANSFER_SIZE", idf, fc);
        idf = idf_make(b->alc, idf_global, get_volt_global(b, "mem", "gc_mark_size"));
        scope_set_idf(class->scope, "GC_MARK_SIZE", idf, fc);
        // MALLOC
        idf = idf_make(b->alc, idf_func, get_volt_func(b, "mem", "alloc"));
        scope_set_idf(class->scope, "MALLOC", idf, fc);
        // MEMCOPY
        idf = idf_make(b->alc, idf_func, get_volt_func(b, "mem", "copy"));
        scope_set_idf(class->scope, "MEMCOPY", idf, fc);

        // Transfer
        strcpy(buf, class->name);
        strcat(buf, "__v_transfer");
        char* name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_transfer");
        char* export_name = dups(alc, buf);
        Func *transfer = func_make(b->alc, class->fc, class->scope, name, export_name);
        transfer->class = class;
        transfer->is_static = false;
        array_push(fc->funcs, transfer);
        map_set_force_new(class->funcs, "_v_transfer", transfer);

        // Mark
        strcpy(buf, class->name);
        strcat(buf, "__v_mark");
        name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_mark");
        export_name = dups(alc, buf);
        Func *mark = func_make(b->alc, class->fc, class->scope, name, export_name);
        mark->class = class;
        mark->is_static = false;
        array_push(fc->funcs, mark);
        map_set_force_new(class->funcs, "_v_mark", mark);

        // AST
        class_generate_transfer(fc, b, class, transfer);
        class_generate_mark(fc, b, class, mark);
    }
}

void class_generate_transfer(Fc* fc, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "(to_state: u8) void {\n");
    // str_flat(code, "  print(\"> Transfer: \")\n");
    // str_flat(code, "  println((this @as ptr).to_hex())\n");
    str_flat(code, "  if @ptrv(this, u8, -8) > 2 { return }\n");
    str_flat(code, "  @ptrv(this, u8, -8) = 4\n");
    str_flat(code, "  @ptrv(this, u8, -7) = 0\n");

    str_flat(code, "  let index = @ptrv(this, u8, -5) @as uint\n");
    str_flat(code, "  let base = (this @as ptr) - (index * (SIZE + 8)) - 8\n");
    str_flat(code, "  let transfer_count = @ptrv(base, uint, -1)\n");
    str_flat(code, "  @ptrv(base, uint, -1) = transfer_count + 1\n");
    str_flat(code, "  GC_TRANSFER_SIZE += SIZE\n");

    // Props
    for(int i = 0; i < props->values->length; i++) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        char var[32];
        strcpy(var, "prop_");
        itoa(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if ");
            str_add(code, var);
            str_flat(code, " != null {\n");
        }
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, "._v_transfer(to_state)\n");
        if(p->type->nullable) {
            str_flat(code, "}\n");
        }
    }

    Func* hook = map_get(class->funcs, "_gc_transfer");
    if(hook) {
        str_flat(code, "  this._gc_transfer()\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(chunk, content, code->length);

    *fc->chunk_parse = *chunk;
    *fc->chunk_parse_prev = *chunk;
    parse_handle_func_args(fc, func);
}
void class_generate_mark(Fc* fc, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "(age: u8) void {\n");
    str_flat(code, "  let state = @ptrv(this, u8, -8)\n");
    str_flat(code, "  if @ptrv(this, u8, -8) > 8 { return }\n");
    str_flat(code, "  if @ptrv(this, u8, -6) == age { return }\n");
    str_flat(code, "  @ptrv(this, u8, -6) = age\n");
    str_flat(code, "  if state == 6 {\n");
    str_flat(code, "    @ptrv(this, u8, -8) = 4\n");
    str_flat(code, "  }\n");

    str_flat(code, "  GC_MARK_SIZE += SIZE\n");
    // Props
    for(int i = 0; i < props->values->length; i++) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        char var[32];
        strcpy(var, "prop_");
        itoa(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if ");
            str_add(code, var);
            str_flat(code, " != null {\n");
        }
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, "._v_mark(age)\n");
        if(p->type->nullable) {
            str_flat(code, "}\n");
        }
    }

    Func* hook = map_get(class->funcs, "_gc_mark");
    if(hook) {
        str_flat(code, "  this._gc_mark()\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(chunk, content, code->length);

    *fc->chunk_parse = *chunk;
    *fc->chunk_parse_prev = *chunk;
    parse_handle_func_args(fc, func);
}

Class* get_generic_class(Fc* fc, Class* class, Map* generic_types) {
    Build* b = fc->b;
    //
    Str* hash = build_get_str_buf(b);
    Array* names = generic_types->keys;
    Array* types = generic_types->values;
    for (int i = 0; i < types->length; i++) {
        char* name = array_get_index(names, i);
        Type* type = array_get_index(types, i);
        type_to_str_append(type, hash);
        str_flat(hash, "|");
    }
    char* h = str_temp_chars(hash);
    Class* gclass = map_get(class->generics, h);
    if (gclass) {
        build_return_str_buf(b, hash);
        return gclass;
    }
    // Generate new
    h = str_to_chars(b->alc, hash);

    // Name
    str_clear(hash);
    str_add(hash, class->name);
    str_flat(hash, "[");
    for (int i = 0; i < types->length; i++) {
        if(i > 0)
            str_flat(hash, ", ");
        char buf[256];
        Type* type = array_get_index(types, i);
        type_to_str(type, buf);
        str_add(hash, buf);
    }
    str_flat(hash, "]");
    char* name = str_to_chars(b->alc, hash);

    // Export name
    str_clear(hash);
    str_add(hash, class->ir_name);
    str_flat(hash, "__");
    for (int i = 0; i < types->length; i++) {
        if(i > 0)
            str_flat(hash, ", ");
        char buf[256];
        Type* type = array_get_index(types, i);
        type_to_str_export(type, buf);
        str_add(hash, buf);
    }
    str_flat(hash, "__");
    char* export_name = str_to_chars(b->alc, hash);

    gclass = class_make(b->alc, b, ct_struct);
    gclass->body = chunk_clone(b->alc, class->body);
    gclass->scope = scope_sub_make(b->alc, sc_default, class->fc->scope, NULL);
    gclass->type = class->type;
    gclass->b = class->b;
    gclass->fc = fc;
    gclass->packed = class->packed;

    gclass->name = name;
    gclass->ir_name = export_name;

    array_push(b->classes, gclass);
    if(gclass->type == ct_class) {
        gclass->gc_vtable_index = ++b->gc_vtables;
    }

    map_set(class->generics, h, gclass);

    // Set type identifiers
    for (int i = 0; i < types->length; i++) {
        char* name = array_get_index(names, i);
        Type* type = array_get_index(types, i);
        Idf* idf = idf_make(b->alc, idf_type, type);
        scope_set_idf(gclass->scope, name, idf, fc);
    }
    // Set CLASS identifier
    Idf* idf = idf_make(b->alc, idf_class, gclass);
    scope_set_idf(gclass->scope, "CLASS", idf, fc);

    // Save chunk for parser
    Chunk ch;
    ch = *fc->chunk_parse;

    // Stage 2
    stage_props_class(fc, gclass);
    // Class size
    int size = class_determine_size(b, gclass);
    if(size == -1) {
        sprintf(b->char_buf, "Cannot determine size of class: '%s'\n", gclass->name);
        parse_err(fc->chunk_parse, b->char_buf);
    }
    // Internals
    class_generate_internals(fc, b, gclass);

    // Types
    stage_types_class(fc, gclass);
    Array* funcs = gclass->funcs->values;
    for (int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        stage_types_func(fc, func);
    }

    // Restore chunk
    *fc->chunk_parse = ch;
    //
    build_return_str_buf(b, hash);
    return gclass;
}
