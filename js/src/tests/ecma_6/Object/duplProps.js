





a = function() { "use strict"; return { foo: 0, foo : 1 }};
assertEq(a().foo, 1);
a = function() { return { foo: 0, foo : 1 }};
assertEq(a().foo, 1);


a = function() { "use strict"; return { foo : 1, get foo() { return 2; }}};
assertEq(a().foo, 2);
a = function() { return { foo : 1, get foo() { return 2;} }};
assertEq(a().foo, 2);


a = function() { "use strict"; return { get foo() { return 2; }, foo : 1 }};
assertEq(a().foo, 1);
a = function() { return { get foo() { return 2; }, foo : 1 }};
assertEq(a().foo, 1);


a = function() { "use strict"; return { foo : 1, set foo(a) { throw 2; }}};
try {
    a().foo = 5;
    throw new Error("2 should be thrown here");
} catch (e) {
    if (e !== 2)
        throw new Error("2 should be thrown here");
}
a = function() { return { foo : 1, set foo(a) { throw 2;} }};
try {
    a().foo = 5;
    throw new Error("2 should be thrown here");
} catch (e) {
    if (e !== 2)
        throw new Error("2 should be thrown here");
}


a = function() { "use strict"; return { get foo() { return 2; }, get foo() { return 3; } }};
assertEq(a().foo, 3);
a = function() { return { get foo() { return 2; }, get foo() { return 3; } }};
assertEq(a().foo, 3);


a = function() { "use strict"; return { set foo(a) { throw 2; }, foo : 1 }};
assertEq(a().foo, 1);
a = function() { return { set foo(a) { throw 2; }, foo : 1 }};
assertEq(a().foo, 1);


a = function() { "use strict"; return { set foo(a) { throw 2; }, set foo(a) { throw 3; }}};
try {
    a().foo = 5;
    throw new Error("3 should be thrown here");
} catch (e) {
    if (e !== 3)
        throw new Error("3 should be thrown here");
}
a = function() { return { set foo(a) { throw 2; }, set foo(a) { throw 3; }}};
try {
    a().foo = 5;
    throw new Error("3 should be thrown here");
} catch (e) {
    if (e !== 3)
        throw new Error("3 should be thrown here");
}


a = function() { "use strict"; return { get foo() { return 2; }, set foo(a) { throw 3; },
            get foo() { return 4; }}};
try {
    assertEq(a().foo, 4);
    a().foo = 5;
    throw new Error("3 should be thrown here");
} catch (e) {
    if (e !== 3)
        throw new Error("3 should be thrown here");
}
a = function() { return { get foo() { return 2; }, set foo(a) { throw 3; },
            get foo() { return 4; }}};
try {
    assertEq(a().foo, 4);
    a().foo = 5;
    throw new Error("3 should be thrown here");
} catch (e) {
    if (e !== 3)
        throw new Error("3 should be thrown here");
}


a = function() { "use strict"; return { set foo(a) { throw 2; }, get foo() { return 4; },
            set foo(a) { throw 3; }}};
try {
    assertEq(a().foo, 4);
    a().foo = 5;
    throw new Error("3 should be thrown here");
} catch (e) {
    if (e !== 3)
        throw new Error("3 should be thrown here");
}
a = function() { return { set foo(a) { throw 2; }, get foo() { return 4; },
            set foo(a) { throw 3; }}};
try {
    assertEq(a().foo, 4);
    a().foo = 5;
    throw new Error("3 should be thrown here");
} catch (e) {
    if (e !== 3)
        throw new Error("3 should be thrown here");
}

reportCompare(0, 0);
