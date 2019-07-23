












































var i = 0;
var bug = 220367;
var summary = 'Regexp conformance test';
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
string = 'a';
pattern = /(a)|(b)/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'a', undefined);
addThis();

status = inSection(2);
string = 'b';
pattern = /(a)|(b)/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, undefined, 'b');
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
  printBugNumber (bug);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
