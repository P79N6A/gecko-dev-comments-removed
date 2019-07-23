


























































var gTestfile = 'regress-87231.js';
var i = 0;
var BUGNUMBER = 87231;
var cnEmptyString = '';
var summary = 'Testing regular expression /(A)?(A.*)/';
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


pattern = /^(A)?(A.*)$/;
status = inSection(1);
string = 'AAA';
actualmatch = string.match(pattern);
expectedmatch = Array('AAA', 'A', 'AA');
addThis();

status = inSection(2);
string = 'AA';
actualmatch = string.match(pattern);
expectedmatch = Array('AA', 'A', 'A');
addThis();

status = inSection(3);
string = 'A';
actualmatch = string.match(pattern);
expectedmatch = Array('A', undefined, 'A'); 
addThis();


pattern = /(A)?(A.*)/;
var strL = 'zxcasd;fl\\\  ^';
var strR = 'aaAAaaaf;lrlrzs';

status = inSection(4);
string =  strL + 'AAA' + strR;
actualmatch = string.match(pattern);
expectedmatch = Array('AAA' + strR, 'A', 'AA' + strR);
addThis();

status = inSection(5);
string =  strL + 'AA' + strR;
actualmatch = string.match(pattern);
expectedmatch = Array('AA' + strR, 'A', 'A' + strR);
addThis();

status = inSection(6);
string =  strL + 'A' + strR;
actualmatch = string.match(pattern);
expectedmatch = Array('A' + strR, undefined, 'A' + strR); 
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
