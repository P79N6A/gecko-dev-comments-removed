














































var bug = 89474;
var summary = "Testing the JS shell doesn't crash on it.item()";
var cnTest = 'it.item()';



test();



function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  tryThis(cnTest); 

  reportCompare('No Crash', 'No Crash', '');

  exitFunc ('test');
}


function tryThis(sEval)
{
  try
  {
    eval(sEval);
  }
  catch(e)
  {
    
  }
}
