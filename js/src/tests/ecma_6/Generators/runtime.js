








function assertSyntaxError(str) {
    assertThrowsInstanceOf(Function(str), SyntaxError);
}


function f() { }
function* g() { yield 1; }
var GeneratorFunctionPrototype = Object.getPrototypeOf(g);
var GeneratorFunction = GeneratorFunctionPrototype.constructor;
var GeneratorObjectPrototype = GeneratorFunctionPrototype.prototype;

var std_iterator = "@@iterator";




function TestGeneratorFunctionInstance() {
    var f_own_property_names = Object.getOwnPropertyNames(f);
    var g_own_property_names = Object.getOwnPropertyNames(g);

    f_own_property_names.sort();
    g_own_property_names.sort();

    assertDeepEq(f_own_property_names, g_own_property_names);
    var i;
    for (i = 0; i < f_own_property_names.length; i++) {
        var prop = f_own_property_names[i];
        var f_desc = Object.getOwnPropertyDescriptor(f, prop);
        var g_desc = Object.getOwnPropertyDescriptor(g, prop);
        assertEq(f_desc.configurable, g_desc.configurable, prop);
        assertEq(f_desc.writable, g_desc.writable, prop);
        assertEq(f_desc.enumerable, g_desc.enumerable, prop);
    }
}
TestGeneratorFunctionInstance();




function TestGeneratorFunctionPrototype() {
    
    assertEq(Object.getPrototypeOf(f), Function.prototype);
    assertNotEq(GeneratorFunctionPrototype, Function.prototype);
    assertEq(Object.getPrototypeOf(GeneratorFunctionPrototype),
               Function.prototype);
    assertEq(Object.getPrototypeOf(function* () {}),
               GeneratorFunctionPrototype);
}
TestGeneratorFunctionPrototype();




function TestGeneratorObjectPrototype() {
    assertEq(Object.getPrototypeOf(GeneratorObjectPrototype),
               Object.prototype);
    assertEq(Object.getPrototypeOf((function*(){yield 1}).prototype),
               GeneratorObjectPrototype);

    var expected_property_names = ["next", "throw", "constructor", std_iterator];
    var found_property_names =
        Object.getOwnPropertyNames(GeneratorObjectPrototype);

    expected_property_names.sort();
    found_property_names.sort();

    assertDeepEq(found_property_names, expected_property_names);
}
TestGeneratorObjectPrototype();




function TestGeneratorFunction() {
    assertEq(GeneratorFunctionPrototype, GeneratorFunction.prototype);
    assertTrue(g instanceof GeneratorFunction);

    assertEq(Function, Object.getPrototypeOf(GeneratorFunction));
    assertTrue(g instanceof Function);

    assertEq("function* g() { yield 1; }", g.toString());

    
    assertTrue(f instanceof Function);  
    assertFalse(f instanceof GeneratorFunction);

    assertTrue((new GeneratorFunction()) instanceof GeneratorFunction);
    assertTrue(GeneratorFunction() instanceof GeneratorFunction);

    assertTrue(GeneratorFunction('yield 1') instanceof GeneratorFunction);
    assertTrue(GeneratorFunction('return 1') instanceof GeneratorFunction);
    assertTrue(GeneratorFunction('a', 'yield a') instanceof GeneratorFunction);
    assertTrue(GeneratorFunction('a', 'return a') instanceof GeneratorFunction);
    assertTrue(GeneratorFunction('a', 'return a') instanceof GeneratorFunction);
    assertSyntaxError("GeneratorFunction('yield', 'return yield')");
    assertTrue(GeneratorFunction('with (x) return foo;') instanceof GeneratorFunction);
    assertSyntaxError("GeneratorFunction('\"use strict\"; with (x) return foo;')");

    
    
    assertEq(GeneratorFunction('yield 10').toString(),
             "function* anonymous() {\nyield 10\n}");
}
TestGeneratorFunction();


function TestPerGeneratorPrototype() {
    assertNotEq((function*(){}).prototype, (function*(){}).prototype);
    assertNotEq((function*(){}).prototype, g.prototype);
    assertEq(typeof GeneratorFunctionPrototype, "object");
    assertEq(g.prototype.__proto__.constructor, GeneratorFunctionPrototype, "object");
    assertEq(Object.getPrototypeOf(g.prototype), GeneratorObjectPrototype);
    assertFalse(g.prototype instanceof Function);
    assertEq(typeof (g.prototype), "object");

    assertDeepEq(Object.getOwnPropertyNames(g.prototype), []);
}
TestPerGeneratorPrototype();


if (typeof reportCompare == "function")
    reportCompare(true, true);
