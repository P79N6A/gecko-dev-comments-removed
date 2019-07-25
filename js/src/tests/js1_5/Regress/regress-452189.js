







var BUGNUMBER = 452189;
var summary = "Don't shadow a readonly or setter proto-property";
var expect = "PASS";
var actual = "FAIL";

function c() {
    this.x = 3;
}


new c;
Object.prototype.__defineSetter__('x', function(){ actual = expect; })
new c;

reportCompare(expect, actual, summary);
