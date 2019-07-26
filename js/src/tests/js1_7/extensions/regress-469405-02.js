





var BUGNUMBER = 469405;
var summary = 'Do not assert: !regs.sp[-2].isPrimitive()';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{ 
  eval("__proto__.__iterator__ = [].toString");
  for (var z = 0; z < 3; ++z) { if (z % 3 == 2) { for(let y in []); } }
}
catch(ex)
{
  print('caught ' + ex);
}

reportCompare(expect, actual, summary);
