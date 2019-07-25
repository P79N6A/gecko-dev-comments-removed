


assertEq(new (Proxy.createFunction({}, function(){}, function(){})), undefined);
x = Proxy.createFunction((function () {}), Uint16Array, wrap)
new(wrap(x))

var x = Proxy.createFunction({}, function (q) { return q; });
assertEq(new x(x), x);
try {
    var x = (Proxy.createFunction({}, "".indexOf));
    new x;
}
catch (e) {
    assertEq(String(e.message).indexOf('is not a constructor') === -1, false);
}
throw "ExitCleanly"
