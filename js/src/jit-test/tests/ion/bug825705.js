

evalcx("\
    var x = newGlobal().Object;\
    function f() { return new x; }\
    f();\
    f();\
", newGlobal());



var O = new Proxy(function() {}, {
    get: function() {
	    throw "get trap";
    }
});

function f() {
  new O();
}

f();
f();
