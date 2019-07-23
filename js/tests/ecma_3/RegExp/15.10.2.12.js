





































var gTestfile = '15.10.2.12.js';

var BUGNUMBER = 378738;
var summary = '15.10.2.12 - CharacterClassEscape \d';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = false;
  actual = /\d/.test("\uFF11");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
