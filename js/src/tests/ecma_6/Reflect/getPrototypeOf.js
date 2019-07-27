



assertEq(Reflect.getPrototypeOf({}), Object.prototype);
assertEq(Reflect.getPrototypeOf(Object.prototype), null);
assertEq(Reflect.getPrototypeOf(Object.create(null)), null);



var proxy = new Proxy({}, {
    getPrototypeOf(t) { return Math; }
});
var result = Reflect.getPrototypeOf(proxy);
if (result === Math) {
    throw new Error("Congratulations on fixing bug 888969! " +
                    "Please update this test to cover scripted proxies.");
}



reportCompare(0, 0);
