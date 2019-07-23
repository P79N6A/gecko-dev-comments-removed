


















































var gTestfile = 'regress-202564.js';
var i = 0;
var BUGNUMBER = 202564;
var summary = 'Testing regexp with many backreferences';
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
string = 'Seattle, WA to Buckley, WA';
pattern = /(?:(.+), )?(.+), (..) to (?:(.+), )?(.+), (..)/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, undefined, "Seattle", "WA", undefined, "Buckley", "WA");
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
