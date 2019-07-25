


var global = this;

(function() {
    function f() {
        this.b = function() {};
        Object.defineProperty(this, "b", ({
            configurable: global.__defineSetter__("", function() {})
        }));
    }
    for each(y in [0]) {
        _ = new f();
    }
})();
uneval(this);

reportCompare(true, true);
