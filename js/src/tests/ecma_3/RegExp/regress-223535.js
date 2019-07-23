














































var gTestfile = 'regress-223535.js';
var i = 0;
var BUGNUMBER = 223535;
var summary = 'Testing regexps with empty alternatives';
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


string = 'a';
status = inSection(1);
pattern = /a|/;
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(2);
pattern = /|a/;
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(3);
pattern = /|/;
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(4);
pattern = /(a|)/;
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a');
addThis();

status = inSection(5);
pattern = /(a||)/;
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a');
addThis();

status = inSection(6);
pattern = /(|a)/;
actualmatch = string.match(pattern);
expectedmatch = Array('', '');
addThis();

status = inSection(7);
pattern = /(|a|)/;
actualmatch = string.match(pattern);
expectedmatch = Array('', '');
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
