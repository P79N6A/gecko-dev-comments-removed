

load(libdir + "asserts.js");
load(libdir + "iteration.js");

function test(constructor) {
    var proto = Object.getPrototypeOf(constructor()[Symbol.iterator]());
    var names = Object.getOwnPropertyNames(proto);
    names.sort();
    assertDeepEq(names, ['next']);
    assertEq(proto.hasOwnProperty(Symbol.iterator), true);

    var desc = Object.getOwnPropertyDescriptor(proto, 'next');
    assertEq(desc.configurable, true);
    assertEq(desc.enumerable, false);
    assertEq(desc.writable, true);

    assertEq(proto[Symbol.iterator](), proto);
    assertIteratorDone(proto, undefined);
}


test(Map);
test(Set);
