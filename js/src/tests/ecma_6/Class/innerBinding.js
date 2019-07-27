


if (classesEnabled()) {




function syntaxWrapper() {
    eval("class Foo { constructor() { } tryBreak() { Foo = 4; } }");
}
assertThrowsInstanceOf(syntaxWrapper, SyntaxError);












var test = `

// TDZ applies to inner bindings
assertThrowsInstanceOf(()=>eval(\`class Bar {
                                    constructor() { };
                                    [Bar] () { };
                                 }\`), ReferenceError);

// There's no magic "inner binding" global
{
    class Foo {
        constructor() { };
        test() {
            class Bar {
                constructor() { }
                test() { return Foo === Bar }
            }
            return new Bar().test();
        }
    }
    assertEq(new Foo().test(), false);
}

// Inner bindings are shadowable
{
    class Foo {
        constructor() { }
        test(Foo) { return Foo; }
    }
    assertEq(new Foo().test(4), 4);
}

// The outer binding is distinct from the inner one
{
    let orig_X;

    class X {
        constructor() { }
        f() { assertEq(X, orig_X); }
    }

    orig_X = X;
    X = 13;
    assertEq(X, 13);
    new orig_X().f();
}
`;

eval(test);

}

if (typeof reportCompare === "function")
    reportCompare(0, 0, "OK");
