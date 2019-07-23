



























































var gTestfile = 'regress-187133.js';
var i = 0;
var BUGNUMBER = 187133;
var summary = 'RegExp conformance test';
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


pattern = /(\.(?!com|org)|\/)/;
status = inSection(1);
string = 'ah.info';
actualmatch = string.match(pattern);
expectedmatch = ['.', '.'];
addThis();

status = inSection(2);
string = 'ah/info';
actualmatch = string.match(pattern);
expectedmatch = ['/', '/'];
addThis();

status = inSection(3);
string = 'ah.com';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();


pattern = /(?!a|b)|c/;
status = inSection(4);
string = '';
actualmatch = string.match(pattern);
expectedmatch = [''];
addThis();

status = inSection(5);
string = 'bc';
actualmatch = string.match(pattern);
expectedmatch = [''];
addThis();

status = inSection(6);
string = 'd';
actualmatch = string.match(pattern);
expectedmatch = [''];
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
