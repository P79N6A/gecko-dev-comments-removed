




































var gTestfile = 'regress-352640-01.js';

var BUGNUMBER = 352640;
var summary = 'Do not assert: scopeStmt or crash @ js_LexicalLookup';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    new Function("do { with({}) let x; let b; } while( 1 );");
  }
  catch(ex)
  {
    print(ex + '');
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
