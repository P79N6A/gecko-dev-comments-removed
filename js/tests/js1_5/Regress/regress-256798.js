





































var bug = 256798;
var summary = 'regexp zero-width positive lookahead';
var actual = '';
var expect = '';

  printBugNumber (bug);
  printStatus (summary);

var status;

status = summary + ' ' + inSection(1);
expect = 'aaaa,a';
actual = /(?:(a)+)/.exec("baaaa") + '';
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2);
expect = ',aaa';
actual = /(?=(a+))/.exec("baaabac") + '';
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3);
expect = 'b,aaa';
actual = /b(?=(a+))/.exec("baaabac") + '';
reportCompare(expect, actual, status);


status = summary + ' ' + inSection(4);
expect = 'null';
actual = /b(?=(b+))/.exec("baaabac") + '';
reportCompare(expect, actual, status);
