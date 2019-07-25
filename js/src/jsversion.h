





#ifndef jsversion_h___
#define jsversion_h___




#ifndef JS_VERSION
#define JS_VERSION 185
#endif





























#define JS_VERSION_ECMA_3       148
#define JS_VERSION_ECMA_3_TEST  149

#if JS_VERSION == JS_VERSION_ECMA_3 ||                                        \
    JS_VERSION == JS_VERSION_ECMA_3_TEST

#define JS_HAS_STR_HTML_HELPERS 0       /* has str.anchor, str.bold, etc. */
#if JS_VERSION == JS_VERSION_ECMA_3_TEST
#define JS_HAS_OBJ_PROTO_PROP   1       /* has o.__proto__ etc. */
#else
#define JS_HAS_OBJ_PROTO_PROP   0       /* has o.__proto__ etc. */
#endif
#define JS_HAS_OBJ_WATCHPOINT   0       /* has o.watch and o.unwatch */
#define JS_HAS_TOSOURCE         0       /* has Object/Array toSource method */
#define JS_HAS_CATCH_GUARD      0       /* has exception handling catch guard */
#define JS_HAS_UNEVAL           0       /* has uneval() top-level function */
#define JS_HAS_CONST            0       /* has JS2 const as alternative var */
#define JS_HAS_FUN_EXPR_STMT    0       /* has function expression statement */
#define JS_HAS_NO_SUCH_METHOD   0       /* has o.__noSuchMethod__ handler */
#define JS_HAS_GENERATORS       0       /* has yield in generator function */
#define JS_HAS_BLOCK_SCOPE      0       /* has block scope via let/arraycomp */
#define JS_HAS_DESTRUCTURING    0       /* has [a,b] = ... or {p:a,q:b} = ... */
#define JS_HAS_GENERATOR_EXPRS  0       /* has (expr for (lhs in iterable)) */
#define JS_HAS_EXPR_CLOSURES    0       /* has function (formals) listexpr */

#elif JS_VERSION < 150

#error "unsupported JS_VERSION"

#elif JS_VERSION == 150

#define JS_HAS_STR_HTML_HELPERS 1       /* has str.anchor, str.bold, etc. */
#define JS_HAS_OBJ_PROTO_PROP   1       /* has o.__proto__ etc. */
#define JS_HAS_OBJ_WATCHPOINT   1       /* has o.watch and o.unwatch */
#define JS_HAS_TOSOURCE         1       /* has Object/Array toSource method */
#define JS_HAS_CATCH_GUARD      1       /* has exception handling catch guard */
#define JS_HAS_UNEVAL           1       /* has uneval() top-level function */
#define JS_HAS_CONST            1       /* has JS2 const as alternative var */
#define JS_HAS_FUN_EXPR_STMT    1       /* has function expression statement */
#define JS_HAS_NO_SUCH_METHOD   1       /* has o.__noSuchMethod__ handler */
#define JS_HAS_GENERATORS       0       /* has yield in generator function */
#define JS_HAS_BLOCK_SCOPE      0       /* has block scope via let/arraycomp */
#define JS_HAS_DESTRUCTURING    0       /* has [a,b] = ... or {p:a,q:b} = ... */
#define JS_HAS_GENERATOR_EXPRS  0       /* has (expr for (lhs in iterable)) */
#define JS_HAS_EXPR_CLOSURES    0       /* has function (formals) listexpr */

#elif JS_VERSION == 160

#define JS_HAS_STR_HTML_HELPERS 1       /* has str.anchor, str.bold, etc. */
#define JS_HAS_OBJ_PROTO_PROP   1       /* has o.__proto__ etc. */
#define JS_HAS_OBJ_WATCHPOINT   1       /* has o.watch and o.unwatch */
#define JS_HAS_TOSOURCE         1       /* has Object/Array toSource method */
#define JS_HAS_CATCH_GUARD      1       /* has exception handling catch guard */
#define JS_HAS_UNEVAL           1       /* has uneval() top-level function */
#define JS_HAS_CONST            1       /* has JS2 const as alternative var */
#define JS_HAS_FUN_EXPR_STMT    1       /* has function expression statement */
#define JS_HAS_NO_SUCH_METHOD   1       /* has o.__noSuchMethod__ handler */
#define JS_HAS_GENERATORS       0       /* has yield in generator function */
#define JS_HAS_BLOCK_SCOPE      0       /* has block scope via let/arraycomp */
#define JS_HAS_DESTRUCTURING    0       /* has [a,b] = ... or {p:a,q:b} = ... */
#define JS_HAS_GENERATOR_EXPRS  0       /* has (expr for (lhs in iterable)) */
#define JS_HAS_EXPR_CLOSURES    0       /* has function (formals) listexpr */

#elif JS_VERSION == 170

#define JS_HAS_STR_HTML_HELPERS 1       /* has str.anchor, str.bold, etc. */
#define JS_HAS_OBJ_PROTO_PROP   1       /* has o.__proto__ etc. */
#define JS_HAS_OBJ_WATCHPOINT   1       /* has o.watch and o.unwatch */
#define JS_HAS_TOSOURCE         1       /* has Object/Array toSource method */
#define JS_HAS_CATCH_GUARD      1       /* has exception handling catch guard */
#define JS_HAS_UNEVAL           1       /* has uneval() top-level function */
#define JS_HAS_CONST            1       /* has JS2 const as alternative var */
#define JS_HAS_FUN_EXPR_STMT    1       /* has function expression statement */
#define JS_HAS_NO_SUCH_METHOD   1       /* has o.__noSuchMethod__ handler */
#define JS_HAS_GENERATORS       1       /* has yield in generator function */
#define JS_HAS_BLOCK_SCOPE      1       /* has block scope via let/arraycomp */
#define JS_HAS_DESTRUCTURING    1       /* has [a,b] = ... or {p:a,q:b} = ... */
#define JS_HAS_GENERATOR_EXPRS  0       /* has (expr for (lhs in iterable)) */
#define JS_HAS_EXPR_CLOSURES    0       /* has function (formals) listexpr */

#elif 180 <= JS_VERSION && JS_VERSION <= 185

#define JS_HAS_STR_HTML_HELPERS 1       /* has str.anchor, str.bold, etc. */
#define JS_HAS_OBJ_PROTO_PROP   1       /* has o.__proto__ etc. */
#define JS_HAS_OBJ_WATCHPOINT   1       /* has o.watch and o.unwatch */
#define JS_HAS_TOSOURCE         1       /* has Object/Array toSource method */
#define JS_HAS_CATCH_GUARD      1       /* has exception handling catch guard */
#define JS_HAS_UNEVAL           1       /* has uneval() top-level function */
#define JS_HAS_CONST            1       /* has JS2 const as alternative var */
#define JS_HAS_FUN_EXPR_STMT    1       /* has function expression statement */
#define JS_HAS_NO_SUCH_METHOD   1       /* has o.__noSuchMethod__ handler */
#define JS_HAS_GENERATORS       1       /* has yield in generator function */
#define JS_HAS_BLOCK_SCOPE      1       /* has block scope via let/arraycomp */
#define JS_HAS_DESTRUCTURING    2       /* has [a,b] = ... or {p:a,q:b} = ... */
#define JS_HAS_GENERATOR_EXPRS  1       /* has (expr for (lhs in iterable)) */
#define JS_HAS_EXPR_CLOSURES    1       /* has function (formals) listexpr */

#else

#error "unknown JS_VERSION"

#endif


#define JS_HAS_NEW_GLOBAL_OBJECT        1


#define JS_HAS_MAKE_SYSTEM_OBJECT       1


#define JS_HAS_DESTRUCTURING_SHORTHAND  (JS_HAS_DESTRUCTURING == 2)





#define OLD_GETTER_SETTER_METHODS       1


#define USE_NEW_OBJECT_REPRESENTATION 0

#if USE_NEW_OBJECT_REPRESENTATION
#  define NEW_OBJECT_REPRESENTATION_ONLY() ((void)0)
#else
#  define NEW_OBJECT_REPRESENTATION_ONLY() \
     MOZ_NOT_REACHED("don't call this!  to be used in the new object representation")
#endif

#endif 
