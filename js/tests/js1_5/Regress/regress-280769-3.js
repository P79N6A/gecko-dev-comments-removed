





































var gTestfile = 'regress-280769-3.js';

var BUGNUMBER = 280769;
var summary = 'Do not crash on overflow of 64K boundary in number of classes in regexp';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var N = 100 * 1000;

status = summary + ' ' + inSection(3) + ' (new RegExp("[0][1]...[99999]").exec("") ';

var a = new Array(N);

for (var i = 0; i != N; ++i) {
  a[i] = i;
}

var str = '['+a.join('][')+']'; 

try
{
  var re = new RegExp(str);
}
catch(e)
{
  printStatus('Exception creating RegExp: ' + e);
}

try
{
  re.exec('');
}
catch(e)
{
  printStatus('Exception executing RegExp: ' + e);
}

reportCompare(expect, actual, status);
