





var BUGNUMBER = 479353;
var summary = 'Do not assert: (uint32_t)(index_) < atoms_->length';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function f(code)
{
  (eval("(function(){" + code + "});"))();
}
x = {};
f("y = this;");
f("x, y; for each (let x in [arguments]) {}");

reportCompare(expect, actual, summary);
