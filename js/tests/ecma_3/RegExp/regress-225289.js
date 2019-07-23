














































var gTestfile = 'regress-225289.js';
var i = 0;
var BUGNUMBER = 225289;
var summary = 'Testing regexps with complementary alternatives';
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



pattern = /a|[^a]/;

status = inSection(1);
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(2);
string = '';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(3);
string = '()';
actualmatch = string.match(pattern);
expectedmatch = Array('(');
addThis();


pattern = /(a|[^a])/;

status = inSection(4);
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a');
addThis();

status = inSection(5);
string = '';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(6);
string = '()';
actualmatch = string.match(pattern);
expectedmatch = Array('(', '(');
addThis();


pattern = /(a)|([^a])/;

status = inSection(7);
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a', undefined);
addThis();

status = inSection(8);
string = '';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(9);
string = '()';
actualmatch = string.match(pattern);
expectedmatch = Array('(', undefined, '(');
addThis();



pattern = /((?:a|[^a])*)/g;

status = inSection(10);
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', ''); 
addThis();

status = inSection(11);
string = '';
actualmatch = string.match(pattern);
expectedmatch = Array(''); 
addThis();

status = inSection(12);
string = '()';
actualmatch = string.match(pattern);
expectedmatch = Array('()', ''); 
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
