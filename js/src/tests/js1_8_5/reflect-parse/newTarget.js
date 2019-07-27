
function testNewTarget() {

    
    assertInFunctionExpr("new.target", newTarget());

    
    assertInFunctionExpr(`new.
                            target`, newTarget());

    
    assertError("new.target", SyntaxError);

    
    assertInFunctionExpr("()=>new.target", arrowExpr([], newTarget()));
    assertError("(() => new.target))", SyntaxError);

    
    assertStmt("function *foo() { new.target; }", genFunDecl(ident("foo"), [],
               blockStmt([exprStmt(newTarget())])));

    
    
    assertInFunctionExpr("new.target.foo", dotExpr(newTarget(), ident("foo")));
    assertInFunctionExpr("new.target[\"foo\"]", memExpr(newTarget(), literal("foo")));

    assertInFunctionExpr("new.target()", callExpr(newTarget(), []));
    assertInFunctionExpr("new new.target()", newExpr(newTarget(), []));

    
    assertError("new.target = 4", SyntaxError);

    
    assertError("new.", SyntaxError);
    assertError("new.foo", SyntaxError);
    assertError("new.targe", SyntaxError);

    
    assertExpr("obj.new.target", dotExpr(dotExpr(ident("obj"), ident("new")), ident("target")));
}

runtest(testNewTarget);
