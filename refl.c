#include "umka_api.h"
#include "umka_types.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum ReflTypeKind {
  RTK_INVALID,
  RTK_OTHER,
  RTK_ENUM,
  RTK_STRUCT,
  RTK_CLOSURE,
  RTK_INTERFACE,
  RTK_PTR,
  RTK_ARR,
  RTK_DYNARR,
  RTK_MAP
};

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
  } else if (type->kind == TYPE_INTERFACE) {
    return RTK_INTERFACE;
  } else if (type->kind == TYPE_PTR || type->kind == TYPE_WEAKPTR) {
    return RTK_PTR;
  } else if (type->kind == TYPE_ARRAY) {
    return RTK_ARR;
  } else if (type->kind == TYPE_DYNARRAY) {
    return RTK_DYNARR;
  } else if (type->kind == TYPE_MAP) {
    return RTK_MAP;
  } else {
    return RTK_OTHER;
  }
}

// From Umka itself --
static inline int64_t align(int64_t size, int64_t alignment) {
  return ((size + (alignment - 1)) / alignment) * alignment;
}

int typeSizeNoCheck(Type *type);

int typeAlignmentNoCheck(Type *type) {
  switch (type->kind) {
  case TYPE_VOID:
    return 1;
  case TYPE_INT8:
  case TYPE_INT16:
  case TYPE_INT32:
  case TYPE_INT:
  case TYPE_UINT8:
  case TYPE_UINT16:
  case TYPE_UINT32:
  case TYPE_UINT:
  case TYPE_BOOL:
  case TYPE_CHAR:
  case TYPE_REAL32:
  case TYPE_REAL:
  case TYPE_PTR:
  case TYPE_WEAKPTR:
  case TYPE_STR:
    return typeSizeNoCheck(type);
  case TYPE_ARRAY:
    return typeAlignmentNoCheck(type->base);
  case TYPE_DYNARRAY:
  case TYPE_MAP:
    return sizeof(int64_t);
  case TYPE_STRUCT:
  case TYPE_INTERFACE:
  case TYPE_CLOSURE: {
    int alignment = 1;
    for (int i = 0; i < type->numItems; i++) {
      const int fieldAlignment = typeAlignmentNoCheck(type->field[i]->type);
      if (fieldAlignment > alignment)
        alignment = fieldAlignment;
    }
    return alignment;
  }
  case TYPE_FIBER:
    return typeSizeNoCheck(type);
  case TYPE_FN:
    return sizeof(int64_t);
  default:
    return 0;
  }
}

int typeSizeNoCheck(Type *type) {
  switch (type->kind) {
  case TYPE_VOID:
    return 0;
  case TYPE_INT8:
    return sizeof(int8_t);
  case TYPE_INT16:
    return sizeof(int16_t);
  case TYPE_INT32:
    return sizeof(int32_t);
  case TYPE_INT:
    return sizeof(int64_t);
  case TYPE_UINT8:
    return sizeof(uint8_t);
  case TYPE_UINT16:
    return sizeof(uint16_t);
  case TYPE_UINT32:
    return sizeof(uint32_t);
  case TYPE_UINT:
    return sizeof(uint64_t);
  case TYPE_BOOL:
    return sizeof(bool);
  case TYPE_CHAR:
    return sizeof(unsigned char);
  case TYPE_REAL32:
    return sizeof(float);
  case TYPE_REAL:
    return sizeof(double);
  case TYPE_PTR:
    return sizeof(void *);
  case TYPE_WEAKPTR:
    return sizeof(uint64_t);
  case TYPE_STR:
    return sizeof(void *);
  case TYPE_ARRAY:
    return type->numItems * typeSizeNoCheck(type->base);
  case TYPE_DYNARRAY:
    return sizeof(DynArray);
  case TYPE_MAP:
    return sizeof(Map);
  case TYPE_STRUCT:
  case TYPE_INTERFACE:
  case TYPE_CLOSURE: {
    int size = 0;
    for (int i = 0; i < type->numItems; i++) {
      const int fieldSize = typeSizeNoCheck(type->field[i]->type);
      size =
          align(size + fieldSize, typeAlignmentNoCheck(type->field[i]->type));
    }
    size = align(size, typeAlignmentNoCheck(type));
    return size;
  }
  case TYPE_FIBER:
    return sizeof(void *);
  case TYPE_FN:
    return sizeof(int64_t);
  default:
    return -1;
  }
}

FN(reflGetTypeSize, {
  Type *type = ARG(0)->ptrVal;

  RET()->uintVal = typeSizeNoCheck(type);
})

FN(reflGetTypeAlignment, {
  Type *type = ARG(0)->ptrVal;

  RET()->uintVal = typeAlignmentNoCheck(type);
})

FN(reflGetStructFieldOffset, {
  Type *type = ARG(0)->ptrVal;
  const char *fieldName = ARG(1)->ptrVal;

  assert(type->kind == TYPE_STRUCT);

  for (int i = 0; i < type->numItems; i++) {
    if (strcmp(type->field[i]->name, fieldName) == 0) {
      RET()->intVal = type->field[i]->offset;
      return;
    }
  }

  RET()->intVal = -1;
})

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
    RET()->ptrVal = api->umkaMakeStr(umka, "");
    break;
  default:
    RET()->ptrVal = api->umkaMakeStr(umka, spelling[type->kind]);
    break;
  }
})

struct Location {
  const char *file;
  int64_t line;
};

FN(reflGetTypeLocation, {
  Type *type = ARG(0)->ptrVal;

  struct Location loc;

  if (type->typeIdent == NULL) {
    loc.file = api->umkaMakeStr(umka, "?");
    loc.line = 0;

    *(struct Location *)RET()->ptrVal = loc;
    return;
  }

  loc.file = api->umkaMakeStr(umka, type->typeIdent->debug.fileName);
  loc.line = type->typeIdent->debug.line;

  *(struct Location *)RET()->ptrVal = loc;
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

FN(reflClosureIsMethod, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_CLOSURE || type->kind == TYPE_FN);

  if (type->kind == TYPE_CLOSURE) {
    RET()->intVal = type->field[0]->type->sig.isMethod;
  } else {
    RET()->intVal = type->sig.isMethod;
  }
})

FN(reflClosureHasUpvalues, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_FN || type->kind == TYPE_CLOSURE);

  RET()->intVal = type->kind == TYPE_CLOSURE;
})

FN(reflGetInterfaceMethods, {
  Type *type = ARG(0)->ptrVal;
  Type *structfieldtype = ARG(1)->ptrVal;
  assert(type->kind == TYPE_INTERFACE);

  UmkaDynArray(StructField) *result = RET()->ptrVal;

  api->umkaMakeDynArray(umka, result, structfieldtype, type->numItems - 2);

  for (int i = 2; i < type->numItems; i++) {
    result->data[i - 2].name = api->umkaMakeStr(umka, type->field[i]->name);
    result->data[i - 2].type = type->field[i]->type;
  }
})

FN(reflGetUnderlyingType, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_PTR || type->kind == TYPE_ARRAY ||
         type->kind == TYPE_DYNARRAY || type->kind == TYPE_MAP);

  if (type->kind == TYPE_MAP) {
    RET()->ptrVal = type->base->field[2]->type->base;
  } else {
    RET()->ptrVal = type->base;
  }
})

FN(reflPointerIsWeak, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_PTR || type->kind == TYPE_WEAKPTR);

  RET()->intVal = type->kind == TYPE_WEAKPTR;
})

FN(reflGetArraySize, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_ARRAY);

  RET()->uintVal = type->numItems;
})

FN(reflGetMapKeyType, {
  Type *type = ARG(0)->ptrVal;
  assert(type->kind == TYPE_MAP);

  RET()->ptrVal = type->base->field[1]->type->base;
})