




































var gTestfile = 'regress-330352.js';

var BUGNUMBER = 330352;
var summary = 'Very non-greedy regexp causes crash in jsregexp.c';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

expectExitCode(0);
expectExitCode(5);

if ("AB".match(/(.*?)*?B/))
{
  printStatus(RegExp.lastMatch);
}
reportCompare(expect, actual, summary + ': "AB".match(/(.*?)*?B/)');

if ("AB".match(/(.*)*?B/))
{
  printStatus(RegExp.lastMatch);
}
reportCompare(expect, actual, summary + ': "AB".match(/(.*)*?B/)');

if ("AB".match(/(.*?)*B/))
{
  printStatus(RegExp.lastMatch);
}
reportCompare(expect, actual, summary + ': "AB".match(/(.*?)*B/)');
