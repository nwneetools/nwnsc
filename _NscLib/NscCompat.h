

/* Bison 3 does not define some of these
   values. YYEMPTY is needed in all cases
   the NWN_BISON_3 macro is defined by the CMakeLists.txt
   based on the version of bison found
*/
#     define YYEMPTY		(-2)
#ifdef NWN_BISON_3
#   define YYLVAL yyla.value
#   define YYCHAR_NAME yyla.type
#else
#   define  YYCHAR_NAME yychar
#   define YYLVAL yylval
#endif


enum nwtoken {
    IDENTIFIER = 258,
    INTEGER_CONST = 259,
    FLOAT_CONST = 260,
    STRING_CONST = 261,
    ADDEQ = 262,
    SUBEQ = 263,
    MULEQ = 264,
    DIVEQ = 265,
    MODEQ = 266,
    XOREQ = 267,
    ANDEQ = 268,
    OREQ = 269,
    SL = 270,
    SR = 271,
    USR = 272,
    SLEQ = 273,
    SREQ = 274,
    USREQ = 275,
    EQ = 276,
    NOTEQ = 277,
    LTEQ = 278,
    GTEQ = 279,
    ANDAND = 280,
    OROR = 281,
    PLUSPLUS = 282,
    MINUSMINUS = 283,
    FLOAT_TYPE = 284,
    INT_TYPE = 285,
    OBJECT_TYPE = 286,
    STRING_TYPE = 287,
    VOID_TYPE = 288,
    STRUCT_TYPE = 289,
    VECTOR_TYPE = 290,
    ENGINE_TYPE = 291,
    ACTION_TYPE = 292,
    OBJECT_SELF_CONST = 293,
    OBJECT_INVALID_CONST = 294,
    BREAK = 295,
    CASE = 296,
    CONTINUE = 297,
    DEFAULT = 298,
    DO = 299,
    ELSE = 300,
    FOR = 301,
    IF = 302,
    RETURN = 303,
    SWITCH = 304,
    WHILE = 305,
    NWCONST = 306
};



