





































var gTestfile = 'regress-256501.js';

var BUGNUMBER = 256501;
var summary = 'Check Recursion';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'error';

try
{ 
  var N = 100*1000;
  var buffer = new Array(N * 2 + 1);
  for (var i = 0; i != N; ++i)
  {
    buffer[i] = 'do ';
    buffer[buffer.length - i - 1] = ' while(0);';
  }
  buffer[N] = 'printStatus("TEST");';

  
  var text = buffer.join('');

  eval(text);
  actual = 'no error';
}
catch(e)
{
  actual = 'error';
}
reportCompare(expect, actual, summary);
