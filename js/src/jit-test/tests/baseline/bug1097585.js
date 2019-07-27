
x = Proxy.createFunction(function() {}, function() {})
function f() {
    x = Proxy.create(function() {}, x())
}
f()
f()
