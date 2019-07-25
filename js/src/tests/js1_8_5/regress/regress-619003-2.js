



var expect = "global";
var actual = expect;
function f([actual]) { }
f(["local"]);
reportCompare(expect, actual, "ok");
