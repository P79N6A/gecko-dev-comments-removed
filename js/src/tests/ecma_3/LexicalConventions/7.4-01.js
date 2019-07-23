




































var gTestfile = '7.4-01.js';

var BUGNUMBER = 475834;
var summary = ' /**/ comments with newlines in them do not act as line breaks';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() {
  L:  for (var i=0; i<2; i++) {
      for (var j=0; j<2; j++) {
        break
L;
      }
      return "conformant!";
    }
    return "non-conformant!";
  }

  expect = 'conformant!';
  print(actual = f());

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
