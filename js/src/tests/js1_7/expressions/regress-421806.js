




































var gTestfile = 'regress-421806.js';

var BUGNUMBER = 421806;
var summary = 'Do not assert: *pcstack[pcdepth - 1] == JSOP_ENTERBLOCK';
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
    let([] = c) 1; 
  } 
  catch(ex) 
  { 
    try
    {
      this.a.b; 
    }
    catch(ex2)
    {
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
