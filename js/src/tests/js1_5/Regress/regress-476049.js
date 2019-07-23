




































var gTestfile = 'regress-476049.js';

var BUGNUMBER = 476049;
var summary = 'JSOP_DEFVAR enables gvar optimization for non-permanent properties';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);




if (typeof window == 'undefined')
{
  print(expect = actual = 'Test skipped');
}
else
{
  document.write(
    '<script type="text/javascript">' +
    'for (var i = 0; i != 1000; ++i)' + 
    '  this["a"+i] = 0;' + 
    'eval("var x");' + 
    'for (var i = 0; i != 1000; ++i)' + 
    '  delete this["a"+i];' + 
    '<\/script>'
    );

  document.write(
    '<script type="text/javascript">' +
    'var x;' + 
    'eval("delete x;");' +
    'x={};' +
    '<\/script>'
    );
}

reportCompare(expect, actual, summary);
