













































var gTestfile = 'regress-169534.js';
var UBound = 0;
var BUGNUMBER = 169534;
var summary = 'RegExp conformance test';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
var re = /(\|)([\w\x81-\xff ]*)(\|)([\/a-z][\w:\/\.]*\.[a-z]{3,4})(\|)/ig;
var str = "To sign up click |here|https://www.xxxx.org/subscribe.htm|";
actual = str.replace(re, '<a href="$4">$2</a>');
expect = 'To sign up click <a href="https://www.xxxx.org/subscribe.htm">here</a>';
addThis();




test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
