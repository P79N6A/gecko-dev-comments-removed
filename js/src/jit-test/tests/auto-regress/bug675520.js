


var handler = {iterate: function() { return Iterator.prototype; }};
var proxy = Proxy.create(handler);
for (var p in proxy) { }
