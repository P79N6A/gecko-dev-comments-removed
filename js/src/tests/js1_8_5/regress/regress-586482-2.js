





var expect = true;
var actual;

var checkCaller = function(me) {
    var caller = arguments.callee.caller;
    var callerIsMethod = (caller === me['doThing']);
    actual = callerIsMethod;
};

Object.prototype.doThing = function() {
    checkCaller(this);
};

["dense"].doThing();

reportCompare(expect, actual, "ok");
