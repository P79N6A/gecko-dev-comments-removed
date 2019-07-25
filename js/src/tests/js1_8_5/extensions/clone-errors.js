



function check(v) {
    try {
        serialize(v);
    } catch (exc) {
        return;
    }
    throw new Error("serializing " + uneval(v) + " should have failed with an exception");
}


check(new Error("oops"));
check(this);
check(Math);
check(function () {});
check(Proxy.create({enumerate: function () { return []; }}));
check(<element/>);
check(new Namespace("x"));
check(new QName("x", "y"));


check({get x() { throw new Error("fail"); }});

reportCompare(0, 0, "ok");
