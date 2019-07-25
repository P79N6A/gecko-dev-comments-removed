



var expect = 'SyntaxError: duplicate argument is mixed with destructuring pattern';
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
