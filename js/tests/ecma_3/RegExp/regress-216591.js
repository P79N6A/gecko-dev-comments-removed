














































var gTestfile = 'regress-216591.js';
var i = 0;
var BUGNUMBER = 216591;
var summary = 'Regexp conformance test';
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
string = 'a {result.data.DATA} b';
pattern = /\{(([a-z0-9\-_]+?\.)+?)([a-z0-9\-_]+?)\}/i;
actualmatch = string.match(pattern);
expectedmatch = Array('{result.data.DATA}', 'result.data.', 'data.', 'DATA');
addThis();













status = inSection(2);
string = 'a {result.data.DATA} b';
pattern = /\{(([a-z0-9\-_]+?\.)+?)([a-z0-9\-_]+?)\}/gi;
actualmatch = string.match(pattern);
expectedmatch = Array('{result.data.DATA}');
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
