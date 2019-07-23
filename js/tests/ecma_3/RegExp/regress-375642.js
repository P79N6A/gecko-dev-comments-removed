




































var bug = 375642;
var summary = 'RegExp /(?:a??)+?/.exec("")';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  /(?:a??)+?/.exec("")

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
