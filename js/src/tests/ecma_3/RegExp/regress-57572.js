















































var gTestfile = 'regress-57572.js';
var i = 0;
var BUGNUMBER = 57572;
var summary = 'Testing regular expressions containing "?"';
var cnEmptyString = ''; var cnSingleSpace = ' ';
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
pattern = /(\S+)?(.*)/;
string = 'Test this';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Test', ' this');  
addThis();

status = inSection(2);
pattern = /(\S+)? ?(.*)/;  
string= 'Test this';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Test', 'this');  
addThis();

status = inSection(3);
pattern = /(\S+)?(.*)/;
string = 'Stupid phrase, with six - (short) words';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Stupid', ' phrase, with six - (short) words');  
addThis();

status = inSection(4);
pattern = /(\S+)? ?(.*)/;  
string = 'Stupid phrase, with six - (short) words';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Stupid', 'phrase, with six - (short) words');  
addThis();



status = inSection(5);
pattern = /(\S+)?( ?)(.*)/;  
string = 'Stupid phrase, with six - (short) words';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Stupid', cnSingleSpace, 'phrase, with six - (short) words');
addThis();

status = inSection(6);
pattern = /^(\S+)?( ?)(B+)$/;  
string = 'AAABBB';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'AAABB', cnEmptyString, 'B');
addThis();

status = inSection(7);
pattern = /(\S+)?(!?)(.*)/;
string = 'WOW !!! !!!';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'WOW', cnEmptyString, ' !!! !!!');
addThis();

status = inSection(8);
pattern = /(.+)?(!?)(!+)/;
string = 'WOW !!! !!!';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'WOW !!! !!', cnEmptyString, '!');
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
