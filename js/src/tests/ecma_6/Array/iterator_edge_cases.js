

function TestArrayIteratorPrototypeConfusion() {
    var iter = [][Symbol.iterator]();
    try {
        iter.next.call(Object.getPrototypeOf(iter))
        throw new Error("Call did not throw");
    } catch (e) {
        assertEq(e instanceof TypeError, true);
        assertEq(e.message, "CallArrayIteratorMethodIfWrapped method called on incompatible Array Iterator");
    }
}
TestArrayIteratorPrototypeConfusion();



function TestArrayIteratorWrappers() {
    var iter = [][Symbol.iterator]();
    assertDeepEq(iter.next.call(newGlobal().eval('[5][Symbol.iterator]()')),
		 { value: 5, done: false })
}
if (typeof newGlobal === "function") {
    TestArrayIteratorWrappers();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
