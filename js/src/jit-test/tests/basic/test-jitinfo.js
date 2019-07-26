

function f() {
    var res = 0;
    var d = new FakeDOMObject();
    assertEq(d !== new FakeDOMObject(), true);

    for (var i=0; i<100; i++) {
	var x = d.x;
	assertEq(typeof x, "number");

	d.x = 10;
	d.x = undefined;

	assertEq(d.doFoo(), 0);
	assertEq(d.doFoo(1), 1);
	assertEq(d.doFoo(1, 2), 2);
    }
}
f();
