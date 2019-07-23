




































var gTestfile = 'regress-418051.js';

var BUGNUMBER = 418051;
var summary = 'Do not assert: (pnkey)->pn_arity == PN_NULLARY && ' + 
  '((pnkey)->pn_type == TOK_NUMBER || (pnkey)->pn_type == TOK_STRING || ' +
  '(pnkey)->pn_type == TOK_NAME)';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    eval("({x:[]}={x}");
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
