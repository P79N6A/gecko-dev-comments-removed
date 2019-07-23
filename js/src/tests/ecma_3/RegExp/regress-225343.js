














































var gTestfile = 'regress-225343.js';
var i = 0;
var BUGNUMBER = 225343;
var summary = 'Testing regexp character classes and the case-insensitive flag';
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
pattern = /[A]/i;
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(2);
string = 'A';
pattern = /[a]/i;
actualmatch = string.match(pattern);
expectedmatch = Array('A');
addThis();

status = inSection(3);
string = '123abc123';
pattern = /([A-Z]+)/i;
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'abc');
addThis();

status = inSection(4);
string = '123abc123';
pattern = /([A-Z])+/i;
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'c');
addThis();

status = inSection(5);
string = 'abc@test.com';
pattern = /^[-!#$%&\'*+\.\/0-9=?A-Z^_`{|}~]+@([-0-9A-Z]+\.)+([0-9A-Z]){2,4}$/i;
actualmatch = string.match(pattern);
expectedmatch = Array('abc@test.com', 'test.', 'm');
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
