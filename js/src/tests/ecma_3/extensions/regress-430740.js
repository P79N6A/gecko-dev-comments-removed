




































var gTestfile = 'regress-430740.js';

var BUGNUMBER = 430740;
var summary = 'Do not strip format-control characters from string literals';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function doevil() {
    print('evildone');
    return 'evildone';
  }

  expect = 'a%E2%80%8D,+doevil()%5D)//';
  actual += eval("(['a\\\u200d', '+doevil()])//'])");
  actual = encodeURI(actual);
  reportCompare(expect, actual, summary);

  expect = 'a%EF%BF%BE,+doevil()%5D)//';
  actual = eval("(['a\\\ufffe', '+doevil()])//'])"); 
  actual = encodeURI(actual);
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
