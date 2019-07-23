














































var gTestfile = 'regress-105972.js';
var i = 0;
var BUGNUMBER = 105972;
var summary = 'Regression test for Bugzilla bug 105972';
var cnEmptyString = '';
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
pattern = /^.*?$/;
string = 'Hello World';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();







status = inSection(2);
pattern = /^.*?/;
string = 'Hello World';
actualmatch = string.match(pattern);
expectedmatch = Array(cnEmptyString);
addThis();











status = inSection(3);
pattern = /^.*?(:|$)/;
string = 'Hello: World';
actualmatch = string.match(pattern);
expectedmatch = Array('Hello:', ':');
addThis();













status = inSection(4);
pattern = /^.*(:|$)/;
string = 'Hello: World';
actualmatch = string.match(pattern);
expectedmatch = Array(string, cnEmptyString);
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
