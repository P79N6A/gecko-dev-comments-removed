



var expect = 'SyntaxError: duplicate argument names not allowed in this context';
var actual = 'No error';

var a = [];


for (var i = 0; i < 200; i++) {
    a.push("b" + i);
    try {
        eval("(function ([" + a.join("],[") + "],a,a){})");
    } catch (e) {
        actual = '' + e;
    }
    assertEq(actual, expect);
}
reportCompare(0, 0, "ok");
