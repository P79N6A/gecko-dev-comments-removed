




































var gTestfile = 'regress-352640-04.js';

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
    new Function("switch(w) { case 2: with({}) let y; case 3: }");
  }
  catch(ex)
  {
    print(ex + '');
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
