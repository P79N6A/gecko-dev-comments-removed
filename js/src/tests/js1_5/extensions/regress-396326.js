




































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
    function f1() { var v; return v.prop }; 
    dis(f1);
    reportCompare(expect, actual, summary + 
                  ': function f1() { var v; return v.prop };');

    function f2(arg) { return arg.prop }; 
    dis(f2);
    reportCompare(expect, actual, summary + 
                  ': function f2(arg) { return arg.prop };');

    function f3() { return this.prop }; 
    dis(f3);
    reportCompare(expect, actual, summary + 
                  ': function f3() { return this.prop };');
  }

  exitFunc ('test');
}
