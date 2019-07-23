




































var bug = 358508;
var summary = 'destructuring-parameters and block local functions';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  try
  {
    (function({0x100000badf00d0: a}) {
      function b() {}
      let c;
    })();
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
