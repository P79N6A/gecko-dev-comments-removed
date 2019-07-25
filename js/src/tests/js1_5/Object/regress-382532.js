






































var BUGNUMBER = 382532;
var summary = 'instanceof,... broken by use of |prototype| in heavyweight constructor';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var prototype;

  function Bug() {
    var func = function () { x; };
    prototype;
  }

  expect = true;
  actual = (new Bug instanceof Bug);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
