


assertEq(new (Proxy.createFunction({}, function(){}, function(){})), undefined);

x = Proxy.createFunction((function () {}), Uint16Array, wrap)
new(wrap(x))


var x = Proxy.createFunction({}, function (q) { return q; });
new x(x);


new (Proxy.createFunction({}, "".indexOf));

throw "ExitCleanly"
