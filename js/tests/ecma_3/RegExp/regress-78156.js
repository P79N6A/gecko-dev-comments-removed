

















































var gTestfile = 'regress-78156.js';
var i = 0;
var BUGNUMBER = 78156;
var summary = 'Testing regular expressions with  ^, $, and the m flag -';
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






string = 'aaa\n789\r\nccc\r\n345';
status = inSection(1);
pattern = /^\d/gm;
actualmatch = string.match(pattern);
expectedmatch = ['7','3'];
addThis();

status = inSection(2);
pattern = /\d$/gm;
actualmatch = string.match(pattern);
expectedmatch = ['9','5'];
addThis();

string = 'aaa\n789\r\nccc\r\nddd';
status = inSection(3);
pattern = /^\d/gm;
actualmatch = string.match(pattern);
expectedmatch = ['7'];
addThis();

status = inSection(4);
pattern = /\d$/gm;
actualmatch = string.match(pattern);
expectedmatch = ['9'];
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
