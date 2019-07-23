




































var gTestfile = 'regress-463259.js';

var BUGNUMBER = 463259;
var summary = 'Do not assert: VALUE_IS_FUNCTION(cx, fval)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
jit(true);

try
{
  (function(){
    eval("(function(){ for (var j=0;j<4;++j) if (j==3) undefined(); })();");
  })();
}
catch(ex)
{
}

jit(false);

reportCompare(expect, actual, summary);
