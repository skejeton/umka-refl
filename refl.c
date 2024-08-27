#include "umka_api.h"
#include "umka_types.h"
#include <assert.h>
#include <stdio.h>

enum ReflTypeKind { RTK_INVALID, RTK_OTHER, RTK_ENUM, RTK_STRUCT, RTK_CLOSURE };

static const char *spelling[] = {
    "none",   "forward", "void",      "null",    "int8",  "int16", "int32",
    "int",    "uint8",   "uint16",    "uint32",  "uint",  "bool",  "char",
    "real32", "real",    "^",         "weak ^",  "[...]", "[]",    "str",
    "map",    "struct",  "interface", "fn |..|", "fiber", "fn"};

#define FN(name, body)                                                         \
  UMKA_EXPORT void name(UmkaStackSlot *p, UmkaStackSlot *r) {                  \
    void *umka = umkaGetInstance(r);                                           \
    UmkaAPI *api = umkaGetAPI(umka);                                           \
    body                                                                       \
  }
#define ARG(i) umkaGetParam(p, i)
#define RET() umkaGetResult(p, r)

enum ReflTypeKind getTypeKind(Type *type) {
  if (type == NULL) {
    return RTK_INVALID;
  } else if (type->isEnum) {
    return RTK_ENUM;
  } else if (type->kind == TYPE_STRUCT) {
    return RTK_STRUCT;
  } else if (type->kind == TYPE_FN || type->kind == TYPE_CLOSURE) {
    return RTK_CLOSURE;
  } else {
    return RTK_OTHER;
  }
}

FN(reflGetTypeKind, {
  Type *type = ARG(0)->ptrVal;

  // @TODO: Check if it's within the type array range
  RET()->intVal = getTypeKind(type);
})

FN(reflGetTypeName, {
  Type *type = ARG(0)->ptrVal;

  if (type->typeIdent) {
    RET()->ptrVal = api->umkaMakeStr(umka, type->typeIdent->name);
    return;
  }

  switch (getTypeKind(type)) {
  case RTK_INVALID:
    RET()->ptrVal = api->umkaMakeStr(umka, "invalid");
    break;
  case RTK_ENUM:
    RET()->ptrVal = api->umkaMakeStr(umka, "enum");
    break;
  default:
    RET()->ptrVal = api->umkaMakeStr(umka, spelling[type->kind]);
    break;
  }
})

FN(reflGetEnumVariantName, {
  Type *type = ARG(0)->ptrVal;
  assert(type->isEnum);

  EnumConst *c = NULL;

  for (int i = 0; i < type->numItems; i++) {
    if (type->enumConst[i]->val.intVal == ARG(1)->intVal) {
      c = type->enumConst[i];
      break;
    }
  }

  // @TODO: Error checking.
  if (!c) {
    RET()->ptrVal = api->umkaMakeStr(umka, "?");
    return;
  }

  RET()->ptrVal = api->umkaMakeStr(umka, c->name);
})

typedef struct {
  const char *name;
  int64_t value;
} EnumVariant;

FN(reflGetEnumVariants, {
  Type *type = ARG(0)->ptrVal;
  Type *enumvarianttype = ARG(1)->ptrVal;
  assert(type->isEnum);

  UmkaDynArray(EnumVariant) *result = RET()->ptrVal;

  api->umkaMakeDynArray(umka, result, enumvarianttype, type->numItems);

  for (int i = 0; i < type->numItems; i++) {
    result->data[i].name = api->umkaMakeStr(umka, type->enumConst[i]->name);
    result->data[i].value = type->enumConst[i]->val.intVal;
  }
})

typedef struct {
  const char *name;
  void *type;
} StructField;

FN(reflGetStructFields, {
  Type *type = ARG(0)->ptrVal;
  Type *structfieldtype = ARG(1)->ptrVal;
  assert(type->kind == TYPE_STRUCT);

  UmkaDynArray(StructField) *result = RET()->ptrVal;

  api->umkaMakeDynArray(umka, result, structfieldtype, type->numItems);

  for (int i = 0; i < type->numItems; i++) {
    result->data[i].name = api->umkaMakeStr(umka, type->field[i]->name);
    result->data[i].type = type->field[i]->type;
  }
})

FN(reflGetClosureReturn, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_FN || type->kind == TYPE_CLOSURE);

  if (type->kind == TYPE_CLOSURE) {
    RET()->ptrVal = type->field[0]->type->sig.resultType;
  } else {
    RET()->ptrVal = type->sig.resultType;
  }
})

FN(reflGetClosureParams, {
  Type *type = ARG(0)->ptrVal;
  Type *structfieldtype = ARG(1)->ptrVal;
  assert(type->kind == TYPE_FN || type->kind == TYPE_CLOSURE);

  UmkaDynArray(StructField) *result = RET()->ptrVal;

  if (type->kind == TYPE_CLOSURE) {
    api->umkaMakeDynArray(umka, result, structfieldtype,
                          type->field[0]->type->sig.numParams - 1);

    for (int i = 1; i < type->field[0]->type->sig.numParams; i++) {
      result->data[i - 1].name =
          api->umkaMakeStr(umka, type->field[0]->type->sig.param[i]->name);
      result->data[i - 1].type = type->field[0]->type->sig.param[i]->type;
    }
  } else {
    api->umkaMakeDynArray(umka, result, structfieldtype, type->sig.numParams);

    for (int i = 0; i < type->sig.numParams; i++) {
      result->data[i].name = api->umkaMakeStr(umka, type->sig.param[i]->name);
      result->data[i].type = type->sig.param[i]->type;
    }
  }
})
