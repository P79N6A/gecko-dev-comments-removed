




































var gTestfile = 'regress-380889.js';


var BUGNUMBER = 380889;
var summary = 'Source disassembler assumes SRC_SWITCH has jump table';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(i)
  {
    switch(i){
    case 1:
    case xyzzy:
    }
  }

  if (typeof dis != 'undefined')
  {
    dis(f);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
