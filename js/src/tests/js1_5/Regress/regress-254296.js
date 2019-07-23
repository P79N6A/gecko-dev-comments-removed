





































var gTestfile = 'regress-254296.js';

var BUGNUMBER = 254296;
var summary = 'javascript regular expression negative lookahead';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = [3].toString();
actual = /^\d(?!\.\d)/.exec('3.A');
if (actual)
{
  actual = actual.toString();
}
 
reportCompare(expect, actual, summary + ' ' + inSection(1));

expect = 'AB';
actual = /(?!AB+D)AB/.exec("AB") + '';
reportCompare(expect, actual, summary + ' ' + inSection(2));
