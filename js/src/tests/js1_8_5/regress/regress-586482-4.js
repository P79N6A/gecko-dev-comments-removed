





var expect, actual;

var obj = {
    f: function() {
        expect = this.g;
        actual = arguments.callee.caller;
        print("Ok");
    }
};

var obj2 = { __proto__: obj, g: function() { this.f(); }};

var obj3 = { __proto__: obj2, h: function() { this.g(); }};

var obj4 = { __proto__: obj3 }

obj4.h();

reportCompare(expect, actual, "ok");
