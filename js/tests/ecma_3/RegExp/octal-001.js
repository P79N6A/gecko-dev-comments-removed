













































var gTestfile = 'octal-001.js';
var i = 0;
var BUGNUMBER = 141078;
var summary = 'Testing octal sequences in regexps';
var status = '';
var statusmessages = new Array();
var pattern = '';
var patterns = new Array();
var string = '';
var strings = new Array();
var actualmatch = '';
var actualmatches = new Array();
var expectedmatch = '';
var expectedmatches = new Array();


status = inSection(1);
pattern = /\240/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();






status = inSection(2);
pattern = /ab\052c/;
string = 'ab*c';
actualmatch = string.match(pattern);
expectedmatch = Array('ab*c');
addThis();

status = inSection(3);
pattern = /ab\052*c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(4);
pattern = /ab(\052)+c/;
string = 'ab****c';
actualmatch = string.match(pattern);
expectedmatch = Array('ab****c', '*');
addThis();

status = inSection(5);
pattern = /ab((\052)+)c/;
string = 'ab****c';
actualmatch = string.match(pattern);
expectedmatch = Array('ab****c', '****', '*');
addThis();

status = inSection(6);
pattern = /(?:\052)c/;
string = 'ab****c';
actualmatch = string.match(pattern);
expectedmatch = Array('*c');
addThis();




test();




function addThis()
{
  statusmessages[i] = status;
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
