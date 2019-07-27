if (typeof window === "undefined") {
    
    
    
    window = this;
}

var res = false;
Object.defineProperty(this, "foo", {configurable: true,
				    get: function() { return this === window; },
				    set: function(v) { res = this === window; }});

(function() {
    for (var i = 0; i < 3000; ++i) {
	window.foo = i;
	assertEq(res, true, "setter");
	assertEq(window.foo, true, "getter");
    }
})();

reportCompare(true, true);
