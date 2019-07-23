




































var gTestfile = 'regress-465862.js';

var BUGNUMBER = 465862;
var summary = 'Do case-insensitive matching correctly in JIT for non-ASCII-letters';

var i = 0;
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






jit(true);

status = inSection(1);
string = '@';
pattern = new RegExp('@', 'i');
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();

status = inSection(2);
string = '`';
pattern = new RegExp('`', 'i');
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();

status = inSection(3);
string = '@';
pattern = new RegExp('`', 'i');
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(4);
string = '`';
pattern = new RegExp('@', 'i');
print(string + ' ' + pattern);
actualmatch = string.match(pattern);
print('z ' + actualmatch);
print('`'.match(/@/i));
expectedmatch = null;
addThis();

jit(false);


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
