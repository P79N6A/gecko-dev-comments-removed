



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





var a = [];
a[0] = a;
check(a);


var b = {};
b.next = b;
check(b);


a[0] = b;
b.next = a;
check(a);
check(b);


a = [];
b = a;
for (var i = 0; i < 10000; i++) {
    b[0] = {};
    b[1] = [];
    b = b[1];
}
b[0] = {owner: a};
b[1] = [];
check(a);

reportCompare(0, 0, "ok");
