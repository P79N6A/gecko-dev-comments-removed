






var BUGNUMBER = 312588;
var summary = 'Do not crash creating infinite array';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus(summary);

var a = new Array();

try
{
  for (var i = 0; i < 1e6; i++)
    (a = new Array(a)).sort();
}
catch(ex)
{
  print(ex + '');
}

reportCompare(expect, actual, summary);
