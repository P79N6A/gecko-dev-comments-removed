





var BUGNUMBER = 406572;
var summary = 'JSOP_CLOSURE unconditionally replaces properties of the variable object - Browser only';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof window != 'undefined')
{
  try {
    actual = "FAIL: Unexpected exception thrown";

    var win = window;
    var windowString = String(window);
    window = 1;
    reportCompare(windowString, String(window), "window should be readonly");

    actual = ""; 

    if (1)
      function window() { return 1; }

    actual = "FAIL: this line should never be reached";

    
    
    window = win;
  } catch (e) {
  }
}
else
{
  expect = actual = 'Test can only run in a Gecko 1.9 browser or later.';
  print(actual);
}
reportCompare(expect, actual, summary);


