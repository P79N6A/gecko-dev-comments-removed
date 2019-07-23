





































var gTestfile = 'regress-280769-2.js';

var BUGNUMBER = 280769;
var summary = 'Do not overflow 64K boundary in treeDepth';
var actual = 'No Crash';
var expect = /No Crash|InternalError: allocation size overflow|InternalError: script stack space quota is exhausted/;
var status;
var result;

printBugNumber(BUGNUMBER);
printStatus (summary);

status = summary + ' ' + inSection(1) + ' (new RegExp("0|...|99999") ';

try
{
  var N = 100 * 1000;
  var a = new Array(N);
  for (var i = 0; i != N; ++i) {
    a[i] = i;
  }
  var str = a.join('|');  
  var re = new RegExp(str);
  re.exec(N - 1);
}
catch(ex)
{
  actual = ex + '';
}

print('Done: ' + actual);

reportMatch(expect, actual, summary);



