




assertDeepEq(Reflect.construct(Object, []), {});
assertDeepEq(Reflect.construct(String, ["hello"]), new String("hello"));


var d = Reflect.construct(Date, [1776, 6, 4]);
assertEq(d instanceof Date, true);
assertEq(d.getFullYear(), 1776);  


var obj = {};
assertEq(Reflect.construct(Object, [obj]), obj);






function f(x) { this.x = x; }
assertDeepEq(Reflect.construct(f, [3]), new f(3));
f.prototype = Array.prototype;
assertDeepEq(Reflect.construct(f, [3]), new f(3));


var bound = f.bind(null, "carrot");
assertDeepEq(Reflect.construct(bound, []), new bound);


function classesEnabled(testCode = "class Foo { constructor() {} }") {
    try {
        new Function(testCode);
        return true;
    } catch (e) {
        if (!(e instanceof SyntaxError))
            throw e;
        return false;
    }
}
if (classesEnabled()) {
    eval(`{
        class Base {
            constructor(...args) {
                this.args = args;
                this.newTarget = new.target;
            }
        }
        //class Derived extends Base {
        //    constructor(...args) { super(...args); }
        //}

        assertDeepEq(Reflect.construct(Base, []), new Base);
        //assertDeepEq(Reflect.construct(Derived, [7]), new Derived(7));
        //g = Derived.bind(null, "q");
        //assertDeepEq(Reflect.construct(g, [8, 9]), new g(8, 9));
    }`);

    if (classesEnabled("class X extends Y { constructor() { super(); } }")) {
        throw new Error("Congratulations on implementing super()! " +
                        "Please uncomment the Derived tests in this file!");
    }
}


var g = newGlobal();
var local = {here: this};
g.eval("function F(arg) { this.arg = arg }");
assertDeepEq(Reflect.construct(g.F, [local]), new g.F(local));



var nonConstructors = [
    {},
    Reflect.construct,  
    x => x + 1,
    Math.max.bind(null, 0),  
    ((x, y) => x > y).bind(null, 0),

    
    
    new Proxy(Reflect.construct, {construct(){}}),
];
for (var obj of nonConstructors) {
    assertThrowsInstanceOf(() => Reflect.construct(obj, []), TypeError);
    assertThrowsInstanceOf(() => Reflect.construct(obj, [], Object), TypeError);
}





function checkNewTarget() {
    assertEq(new.target, expected);
    expected = undefined;
}
var expected = checkNewTarget;
Reflect.construct(checkNewTarget, []);


var constructors = [Object, Function, f, bound];
for (var ctor of constructors) {
    expected = ctor;
    Reflect.construct(checkNewTarget, [], ctor);
    assertEq(expected, undefined);
}


for (var v of SOME_PRIMITIVE_VALUES.concat(nonConstructors)) {
    assertThrowsInstanceOf(() => Reflect.construct(checkNewTarget, [], v), TypeError);
}



function someConstructor() {}
var result = Reflect.construct(Array, [], someConstructor);
assertEq(Reflect.getPrototypeOf(result),
         Array.prototype, 
        "Congratulations on implementing Array subclassing! Fix this test for +1 karma point.");
assertEq(result.length, 0);
assertEq(Array.isArray(result), true);




reportCompare(0, 0);
