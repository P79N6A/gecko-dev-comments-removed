


var p = Proxy.createFunction({}, function(x, y) { undefined.x(); });
print(new p(1, 2));

