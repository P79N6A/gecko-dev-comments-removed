
function test() {


assertGlobalDecl("let {x:y} = foo;", varDecl([{ id: objPatt([assignProp("x", ident("y"))]),
                                                init: ident("foo") }]));

assertLocalDecl("let {x:y} = foo;", letDecl([{ id: objPatt([assignProp("x", ident("y"))]),
                                               init: ident("foo") }]));

assertBlockDecl("let {x:y} = foo;", letDecl([{ id: objPatt([assignProp("x", ident("y"))]),
                                               init: ident("foo") }]));

assertDecl("const {x:y} = foo;", constDecl([{ id: objPatt([assignProp("x", ident("y"))]),
                                              init: ident("foo") }]));



assertStmt("let (x=1) { }", letStmt([{ id: ident("x"), init: lit(1) }], blockStmt([])));
assertStmt("let (x=1,y=2) { }", letStmt([{ id: ident("x"), init: lit(1) },
                                         { id: ident("y"), init: lit(2) }],
                                        blockStmt([])));
assertStmt("let (x=1,y=2,z=3) { }", letStmt([{ id: ident("x"), init: lit(1) },
                                             { id: ident("y"), init: lit(2) },
                                             { id: ident("z"), init: lit(3) }],
                                            blockStmt([])));
assertStmt("let (x) { }", letStmt([{ id: ident("x"), init: null }], blockStmt([])));
assertStmt("let (x,y) { }", letStmt([{ id: ident("x"), init: null },
                                     { id: ident("y"), init: null }],
                                    blockStmt([])));
assertStmt("let (x,y,z) { }", letStmt([{ id: ident("x"), init: null },
                                       { id: ident("y"), init: null },
                                       { id: ident("z"), init: null }],
                                      blockStmt([])));
assertStmt("let (x = 1, y = x) { }", letStmt([{ id: ident("x"), init: lit(1) },
                                              { id: ident("y"), init: ident("x") }],
                                             blockStmt([])));
assertError("let (x = 1, x = 2) { }", TypeError);

}

runtest(test);
