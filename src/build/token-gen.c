
#include "../all.h"

Token *token_make(Allocator *alc, int type, void *item) {
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;
    return t;
}

Token *tgen_assign(Allocator *alc, Value *left, Value *right) {
    VPair *pair = al(alc, sizeof(VPair));
    pair->left = left;
    pair->right = right;
    return token_make(alc, t_assign, pair);
}

Token *tgen_return(Allocator *alc, Value *value) {
    return token_make(alc, t_return, value);
}