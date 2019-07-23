

















































































var gTestfile = '15.10.6.2-1.js';
var i = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing regexps with the global flag set';
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
string = 'a b c d e';
pattern = /\w\s\w/g;
actualmatch = string.match(pattern);
expectedmatch = ['a b','c d']; 
addThis();


status = inSection(2);
string = '12345678';
pattern = /\d\d\d/g;
actualmatch = string.match(pattern);
expectedmatch = ['123','456'];
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
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
