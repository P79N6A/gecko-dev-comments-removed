















































gTestfile = 'regress-144834.js';
var BUGNUMBER = 144834;
var summary = 'Local var having same name as switch label inside function';

print(BUGNUMBER);
print(summary);


function RedrawSched()
{
  var MinBound;

  switch (i)
  {
  case MinBound :
  }
}





var s = '';
s += 'function RedrawSched()';
s += '{';
s += '  var MinBound;';
s += '';
s += '  switch (i)';
s += '  {';
s += '    case MinBound :';
s += '  }';
s += '}';
eval(s);

AddTestCase('Do not crash', 'No Crash', 'No Crash');
test();
