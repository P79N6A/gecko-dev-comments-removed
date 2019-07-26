load(libdir + "asserts.js");






var target = function () {};
var handler = {
    construct: function (target1, args) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(args.length, 2);
        assertEq(args[0], 2);
        assertEq(args[1], 3);
    }
}
assertThrowsInstanceOf(function () {new (new Proxy(target, handler))(2, 3)}, TypeError);
