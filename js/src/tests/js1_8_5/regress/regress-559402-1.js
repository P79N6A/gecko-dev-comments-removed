



var expect = "No error";
var actual = expect;

if (typeof options == "function") {
    var opts = options();
    if (!/\bstrict\b/.test(opts))
        options("strict");
    if (!/\bwerror\b/.test(opts))
        options("werror");
}

try {
    eval('function foo() { "use strict"; }');
} catch (e) {
    actual = '' + e;
}

reportCompare(expect, actual, "ok");
