




































var gTestfile = 'regress-452498-053.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);









  let (a=0, b=1, c=2) {}







  try
  {
    @foo; 0;
  }
  catch(ex)
  {
  }





  try
  {
    for (var [x] = x in y) var x;
  }
  catch(ex)
  {
  }


  try
  {
    if (true && @foo) ;
  }
  catch(ex)
  {
  }






  for (var z = 0; z < 3; z++) { (new function(){}); }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
