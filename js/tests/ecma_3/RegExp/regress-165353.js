











































var i = 0;
var bug = 165353;
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


pattern = /^([a-z]+)*[a-z]$/;
  status = inSection(1);
  string = 'a';
  actualmatch = string.match(pattern);
  expectedmatch = Array('a', undefined);
  addThis();

  status = inSection(2);
  string = 'ab';
  actualmatch = string.match(pattern);
  expectedmatch = Array('ab', 'a');
  addThis();

  status = inSection(3);
  string = 'abc';
  actualmatch = string.match(pattern);
  expectedmatch = Array('abc', 'ab');
  addThis();


string = 'www.netscape.com';
  status = inSection(4);
  pattern = /^(([a-z]+)*[a-z]\.)+[a-z]{2,}$/;
  actualmatch = string.match(pattern);
  expectedmatch = Array('www.netscape.com', 'netscape.', 'netscap');
  addThis();

  
  status = inSection(5);
  pattern = /^(([a-z]+)*([a-z])\.)+[a-z]{2,}$/;
  actualmatch = string.match(pattern);
  expectedmatch = Array('www.netscape.com', 'netscape.', 'netscap', 'e');
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
  printBugNumber (bug);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
