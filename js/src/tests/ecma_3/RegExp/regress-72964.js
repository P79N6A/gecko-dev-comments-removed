














































var gTestfile = 'regress-72964.js';
var i = 0;
var BUGNUMBER = 72964;
var summary = 'Testing regular expressions containing non-Latin1 characters';
var cnSingleSpace = ' ';
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


pattern = /[\S]+/;

status = inSection(1);
string = '\u00BF\u00CD\u00BB\u00A7';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


status = inSection(2);
string = '\u00BF\u00CD \u00BB\u00A7';
actualmatch = string.match(pattern);
expectedmatch = Array('\u00BF\u00CD');
addThis();



status = inSection(3);
string = '\u4e00\uac00\u4e03\u4e00';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


status = inSection(4);
string = '\u4e00\uac00 \u4e03\u4e00';
actualmatch = string.match(pattern);
expectedmatch = Array('\u4e00\uac00');
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
