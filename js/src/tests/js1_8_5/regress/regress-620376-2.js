





function test() {
    var p = Proxy.create({ keys: function() { return { get length() { throw 1; }}; }});

    try {
	for (i in p);
	throw new Error("an exception should be thrown");
    } catch (e) {
	assertEq(e, 1);
    }
}

test();
reportCompare(0, 0, "ok");
