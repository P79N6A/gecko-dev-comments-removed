




































var gTestfile = 'regress-396326.js';

var BUGNUMBER = 396326;
var summary = 'Do not assert trying to disassemble get(var|arg) prop';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof dis == 'undefined')
  {
    print('disassembly not supported. test skipped.');
    reportCompare(expect, actual, summary);
  }
  else
  {
    function f4() { let local; return local.prop }; 
    dis(f4);
    reportCompare(expect, actual, summary + 
                  ': function f4() { let local; return local.prop };');
  }

  exitFunc ('test');
}
