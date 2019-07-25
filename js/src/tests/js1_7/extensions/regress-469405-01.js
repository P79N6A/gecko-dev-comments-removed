





































var BUGNUMBER = 469405;
var summary = 'Do not assert: !JSVAL_IS_PRIMITIVE(regs.sp[-2])';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{ 
  (function() {
    var a, b;
    for each (a in [{}, {__iterator__: function(){}}]) for (b in a) { }
  })();
}
catch(ex)
{
  print('caught ' + ex);
}

reportCompare(expect, actual, summary);
