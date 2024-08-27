#include <stdint.h>

enum
{
    DEFAULT_STR_LEN     = 255,
    MAX_IDENT_LEN       = DEFAULT_STR_LEN,
    MAX_IDENTS_IN_LIST  = 256,
    MAX_MODULES         = 1024,
    MAX_PARAMS          = 16,
    MAX_BLOCK_NESTING   = 100,
    MAX_GOTOS           = 100,
};

enum
{
    MAP_NODE_FIELD_LEN   = 0,
    MAP_NODE_FIELD_KEY   = 1,
    MAP_NODE_FIELD_DATA  = 2,
    MAP_NODE_FIELD_LEFT  = 3,
    MAP_NODE_FIELD_RIGHT = 4,
};


typedef struct
{
    int64_t len, capacity;
} StrDimensions;


typedef StrDimensions DynArrayDimensions;


typedef struct
{
    // Must have 8 byte alignment
    struct tagType *type;
    int64_t itemSize;           // Duplicates information contained in type, but useful for better performance
    void *data;                 // Allocated chunk should start at (char *)data - sizeof(DynArrayDimensions)
} DynArray;


typedef struct
{
    // The C equivalent of the Umka interface type
    void *self;
    struct tagType *selfType;
    // Methods are omitted - do not use sizeof() for non-empty interfaces
} Interface;


typedef struct
{
    // The C equivalent of the Umka closure type
    int64_t entryOffset;
    Interface upvalue;      // No methods - equivalent to "any"
} Closure;


typedef struct tagMapNode
{
    // The C equivalent of the Umka map base type
    int64_t len;            // Non-zero for the root node only
    void *key, *data;
    struct tagMapNode *left, *right;
} MapNode;


typedef struct
{
    // Must have 8 byte alignment
    struct tagType *type;
    MapNode *root;
} Map;


typedef struct
{
    char *fileName;
    char *fnName;
    int line;
} DebugInfo;


typedef void (*WarningCallback)(void * /*UmkaError*/ warning);


typedef struct      // Must be identical to UmkaError
{
    char *fileName;
    char *fnName;
    int line, pos, code;
    char *msg;
} ErrorReport;


typedef struct tagStorageChunk
{
    char *data;
    struct tagStorageChunk *next;
} StorageChunk;


typedef struct
{
    StorageChunk *first, *last;
} Storage;


typedef struct
{
    char path[DEFAULT_STR_LEN + 1], folder[DEFAULT_STR_LEN + 1], name[DEFAULT_STR_LEN + 1];
    unsigned int pathHash;
    void *implLib;
    char *importAlias[MAX_MODULES];
    bool isCompiled;
} Module;


typedef struct
{
    char path[DEFAULT_STR_LEN + 1], folder[DEFAULT_STR_LEN + 1], name[DEFAULT_STR_LEN + 1];
    unsigned int pathHash;
    char *source;
} ModuleSource;

typedef struct
{
    int block;
    struct tagIdent *fn;
    int localVarSize;           // For function blocks only
    bool hasReturn;
} BlockStackSlot;


typedef struct tagExternal
{
    char name[DEFAULT_STR_LEN + 1];
    unsigned int hash;
    void *entry;
    bool resolved;
    struct tagExternal *next;
} External;


typedef struct
{
    External *first, *last;
} Externals;


typedef struct
{
    int64_t numParams;
    int64_t numResultParams;
    int64_t numParamSlots;
    int64_t firstSlotIndex[];
} ExternalCallParamLayout;

typedef enum
{
    TOK_NONE,

    // Keywords
    TOK_BREAK,
    TOK_CASE,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_DEFAULT,
    TOK_ELSE,
    TOK_ENUM,
    TOK_FN,
    TOK_FOR,
    TOK_IMPORT,
    TOK_INTERFACE,
    TOK_IF,
    TOK_IN,
    TOK_MAP,
    TOK_RETURN,
    TOK_STR,
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_TYPE,
    TOK_VAR,
    TOK_WEAK,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_MOD,
    TOK_AND,
    TOK_OR,
    TOK_XOR,
    TOK_SHL,
    TOK_SHR,
    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_MULEQ,
    TOK_DIVEQ,
    TOK_MODEQ,
    TOK_ANDEQ,
    TOK_OREQ,
    TOK_XOREQ,
    TOK_SHLEQ,
    TOK_SHREQ,
    TOK_ANDAND,
    TOK_OROR,
    TOK_PLUSPLUS,
    TOK_MINUSMINUS,
    TOK_EQEQ,
    TOK_LESS,
    TOK_GREATER,
    TOK_EQ,
    TOK_QUESTION,
    TOK_NOT,
    TOK_NOTEQ,
    TOK_LESSEQ,
    TOK_GREATEREQ,
    TOK_COLONEQ,
    TOK_LPAR,
    TOK_RPAR,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_CARET,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_COLONCOLON,
    TOK_PERIOD,
    TOK_ELLIPSIS,

    // Other tokens
    TOK_IDENT,
    TOK_INTNUMBER,
    TOK_REALNUMBER,
    TOK_CHARLITERAL,
    TOK_STRLITERAL,

    TOK_IMPLICIT_SEMICOLON,
    TOK_EOLN,
    TOK_EOF
} TokenKind;


typedef char IdentName[MAX_IDENT_LEN + 1];


typedef struct
{
    TokenKind kind;
    union
    {
        struct
        {
            IdentName name;
            unsigned int hash;
        };
        int64_t intVal;
        uint64_t uintVal;
        double realVal;
        char *strVal;
    };
    int line, pos;
} Token;

typedef enum
{
    TYPE_NONE,
    TYPE_FORWARD,
    TYPE_VOID,
    TYPE_NULL,          // Base type for 'null' constant only
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_REAL32,
    TYPE_REAL,
    TYPE_PTR,
    TYPE_WEAKPTR,       // Actually a handle that stores the heap page ID and the offset within the page: (pageId << 32) | pageOffset
    TYPE_ARRAY,
    TYPE_DYNARRAY,
    TYPE_STR,           // Pointer of a special kind that admits assignment of string literals, concatenation and comparison by content
    TYPE_MAP,
    TYPE_STRUCT,
    TYPE_INTERFACE,
    TYPE_CLOSURE,
    TYPE_FIBER,         // Pointer of a special kind
    TYPE_FN
} TypeKind;


typedef union
{
    int64_t intVal;
    uint64_t uintVal;
    void *ptrVal;
    uint64_t weakPtrVal;
    double realVal;
} Const;


typedef struct
{
    IdentName name;
    unsigned int hash;
    struct tagType *type;
    int offset;
} Field;


typedef struct
{
    IdentName name;
    unsigned int hash;
    Const val;
} EnumConst;


typedef struct
{
    IdentName name;
    unsigned int hash;
    struct tagType *type;
    Const defaultVal;
} Param;

typedef struct
{
    int numParams, numDefaultParams;
    bool isMethod;
    int offsetFromSelf;                     // For interface methods
    Param *param[MAX_PARAMS];
    struct tagType *resultType;
} Signature;


typedef struct tagType
{
    TypeKind kind;
    int block;
    struct tagType *base;                       // For pointers, arrays and maps (for maps, denotes the tree node type)
    int numItems;                               // For arrays, structures and interfaces
    bool isExprList;                            // For structures that represent expression lists
    bool isVariadicParamList;                   // For dynamic arrays of interfaces that represent variadic parameter lists
    bool isEnum;                                // For enumerations
    struct tagIdent *typeIdent;                 // For types that have identifiers
    union
    {
        Field **field;                          // For structures, interfaces and closures
        EnumConst **enumConst;                  // For enumerations
        Signature sig;                          // For functions, including methods
    };
    struct tagType *next;
} Type;


typedef struct tagVisitedTypePair
{
    Type *left, *right;
    struct tagVisitedTypePair *next;
} VisitedTypePair;


typedef enum
{
    // Built-in functions are treated specially, all other functions are either constants or variables of "fn" type
    IDENT_CONST,
    IDENT_VAR,
    IDENT_TYPE,
    IDENT_BUILTIN_FN,
    IDENT_MODULE
} IdentKind;


typedef struct tagIdent
{
    IdentKind kind;
    IdentName name;
    unsigned int hash;
    Type *type;
    int module, block;                  // Place of definition (global identifiers are in block 0)
    bool exported, globallyAllocated, used, temporary;
    int prototypeOffset;                // For function prototypes
    union
    {
        void *ptr;                      // For global variables
        int64_t offset;                 // For functions (code offset) or local variables (stack offset)
        Const constant;                 // For constants
        int64_t moduleVal;              // For modules
    };
    DebugInfo debug;
    struct tagIdent *next;
} Ident;

