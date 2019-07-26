

load(libdir + "asserts.js");

assertThrowsInstanceOf(
    function () { Function("(a = function (obj) { with (obj) f(); }) => a()"); },
    SyntaxError);
