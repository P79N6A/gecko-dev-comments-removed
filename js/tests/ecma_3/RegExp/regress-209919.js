














































var gTestfile = 'regress-209919.js';
var i = 0;
var BUGNUMBER = 209919;
var summary = 'Testing regexp submatches with quantifiers';
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
pattern = /(a|b*)*/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'a');
addThis();







status = inSection(2);
string = 'a';
pattern = /(a|b*){5,}/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '');
addThis();









status = inSection(3);
string = 'a';
pattern = /(b*)*/;
actualmatch = string.match(pattern);
expectedmatch = Array('', undefined);
addThis();







status = inSection(4);
string = 'a';
pattern = /(b*)+/;
actualmatch = string.match(pattern);
expectedmatch = Array('', '');
addThis();





pattern = /^\-?(\d{1,}|\.{0,})*(\,\d{1,})?$/;

status = inSection(5);
string = '100.00';
actualmatch = string.match(pattern);
expectedmatch = Array(string, '00', undefined);
addThis();

status = inSection(6);
string = '100,00';
actualmatch = string.match(pattern);
expectedmatch = Array(string, '100', ',00');
addThis();

status = inSection(7);
string = '1.000,00';
actualmatch = string.match(pattern);
expectedmatch = Array(string, '000', ',00');
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
