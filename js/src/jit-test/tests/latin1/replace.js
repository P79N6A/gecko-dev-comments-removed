function testDollarReplacement() {
    
    var s = toLatin1("Foobarbaz123");
    var pat = toLatin1("bar");
    assertEq(s.replace(pat, toLatin1("AA")), "FooAAbaz123");
    assertEq(s.replace(pat, toLatin1("A$$A")), "FooA$Abaz123");
    assertEq(s.replace(pat, toLatin1("A$`A")), "FooAFooAbaz123");
    assertEq(s.replace(pat, toLatin1("A$&A")), "FooAbarAbaz123");
    assertEq(s.replace(pat, toLatin1("A$'A")), "FooAbaz123Abaz123");

    
    assertEq(s.replace(pat, "A\u1200"), "FooA\u1200baz123");
    assertEq(s.replace(pat, "A$$\u1200"), "FooA$\u1200baz123");
    assertEq(s.replace(pat, "A$`\u1200"), "FooAFoo\u1200baz123");
    assertEq(s.replace(pat, "A$&\u1200"), "FooAbar\u1200baz123");
    assertEq(s.replace(pat, "A$'\u1200"), "FooAbaz123\u1200baz123");

    
    s = "Foobarbaz123\u1200";
    assertEq(s.replace(pat, toLatin1("A")), "FooAbaz123\u1200");
    assertEq(s.replace(pat, toLatin1("A$$")), "FooA$baz123\u1200");
    assertEq(s.replace(pat, toLatin1("A$`")), "FooAFoobaz123\u1200");
    assertEq(s.replace(pat, toLatin1("A$&")), "FooAbarbaz123\u1200");
    assertEq(s.replace(pat, toLatin1("A$'")), "FooAbaz123\u1200baz123\u1200");

    
    s = "Foobar\u1200baz123";
    pat += "\u1200";
    assertEq(s.replace(pat, toLatin1("AB")), "FooABbaz123");
    assertEq(s.replace(pat, toLatin1("A$$B")), "FooA$Bbaz123");
    assertEq(s.replace(pat, toLatin1("A$`B")), "FooAFooBbaz123");
    assertEq(s.replace(pat, toLatin1("A$&B")), "FooAbar\u1200Bbaz123");
    assertEq(s.replace(pat, toLatin1("A$'B")), "FooAbaz123Bbaz123");

    
    assertEq(s.replace(pat, "A\u1300"), "FooA\u1300baz123");
    assertEq(s.replace(pat, "A$$\u1300"), "FooA$\u1300baz123");
    assertEq(s.replace(pat, "A$`\u1300"), "FooAFoo\u1300baz123");
    assertEq(s.replace(pat, "A$&\u1300"), "FooAbar\u1200\u1300baz123");
    assertEq(s.replace(pat, "A$'\u1300"), "FooAbaz123\u1300baz123");
}
testDollarReplacement();
