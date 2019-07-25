
assertEq(new (Proxy.createFunction({}, function(){}, function(){})), undefined);


var x = Proxy.createFunction({}, function (q) { return q; });
new x(x);
