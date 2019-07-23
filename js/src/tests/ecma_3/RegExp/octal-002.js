































































var gTestfile = 'octal-002.js';
var i = 0;
var BUGNUMBER = 141078;
var summary = 'Testing regexps containing octal escape sequences';
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
pattern = /.\011/;
string = 'a' + String.fromCharCode(0) + '11';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();








status = inSection(2);
pattern = /.\0xx/;
string = 'a' + String.fromCharCode(0) + 'xx';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();







status = inSection(3);
pattern = /.\0xx/;
string = 'a\0xx';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();






status = inSection(4);
pattern = /.\011/;
string = 'a\011';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();






status = inSection(5);
pattern = /.\011/;
string = 'a\u0009';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();






status = inSection(6);
pattern = /.\011/;
string = 'a\x09';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();






status = inSection(7);
pattern = /.\011/;
string = 'a\t';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();










status = inSection(8);
string = 'a' + String.fromCharCode(0) + '11';
pattern = RegExp(string);
actualmatch = string.match(pattern);
expectedmatch = Array(string);
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
