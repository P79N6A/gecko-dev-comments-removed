



if (typeof findReferences == "function") {
    function C() {}
    var o = new C;
    o.x = {};               
    o[42] = {};             
    o[123456789] = {};      
    o.myself = o;           
    o.alsoMyself = o;       

    assertEq(referencesVia(o, 'group; group_proto', C.prototype), true);
    assertEq(referencesVia(o, 'shape; base; parent', this), true);
    assertEq(referencesVia(o, 'x', o.x), true);
    assertEq(referencesVia(o, 'objectElements[42]', o[42]), true);
    assertEq(referencesVia(o, '123456789', o[123456789]), true);
    assertEq(referencesVia(o, 'myself', o), true);
    assertEq(referencesVia(o, 'alsoMyself', o), true);

    function g() { return 42; }
    function s(v) { }
    var p = Object.defineProperty({}, 'a', { get:g, set:s });
    assertEq(referencesVia(p, 'shape; getter', g), true);
    assertEq(referencesVia(p, 'shape; setter', s), true);

    
    
    
    var q = Object.defineProperty({}, 'a', { get:g, set:s });
    assertEq(referencesVia(p, 'shape; getter', g), true);
    assertEq(referencesVia(q, 'shape; getter', g), true);

    
    
    p.b = 9;
    q.b = 9;
    assertEq(referencesVia(p, 'shape; parent; getter', g), true);
    assertEq(referencesVia(q, 'shape; parent; getter', g), true);

    
    assertEq(referencesVia(C, 'prototype', Object.getPrototypeOf(o)), true);
    assertEq(referencesVia(Object.getPrototypeOf(o), 'constructor', C), true);

    
    a = [];
    a[1] = o;
    assertEq(referencesVia(a, 'objectElements[1]', o), true);

    reportCompare(true, true);
} else {
    reportCompare(true, true, "test skipped: findReferences is not a function");
}
