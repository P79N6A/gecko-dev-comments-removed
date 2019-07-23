












































var bug = 98306;
var summary = "Testing that we don't crash on this code -";
var cnUBOUND = 10;
var re;
var s;



test();



function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  s = '"Hello".match(/[/]/)';
  tryThis(s);

  s = 're = /[/';
  tryThis(s);

  s = 're = /[/]/';
  tryThis(s);

  s = 're = /[//]/';
  tryThis(s);

  reportCompare('No Crash', 'No Crash', '');
  exitFunc ('test');
}



function tryThis(sCode)
{
  
  for (var i=0; i<cnUBOUND; i++)
  {
    try
    {
      eval(sCode);
    }
    catch(e)
    {
      
    }
  }
}
