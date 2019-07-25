







var BUGNUMBER = 503860;
var summary = "Don't shadow a readonly or setter proto-property";
var expect = "PASS";
var actual = "FAIL";
var a = {y: 1};

function B(){}
B.prototype.__defineSetter__('x', function setx(val) { actual = expect; });
var b = new B;
b.y = 1;

var arr = [a, b];       
for each (var obj in arr)
    obj.x = 2;          

reportCompare(expect, actual, summary);
