


















































var gTestfile = 'regress-228087.js';
var i = 0;
var BUGNUMBER = 228087;
var summary = 'Testing regexps with unescaped braces';
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
var e;


string = 'foo {1} foo {2} foo';


status = inSection(1);
try
{
  pattern = new RegExp('\{1.*\}', 'g');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('{1} foo {2}');
addThis();


status = inSection(2);
try
{
  pattern = new RegExp('{1.*}', 'g');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('{1} foo {2}');
addThis();


status = inSection(3);
try
{
  pattern = new RegExp('\{1[.!\}]*\}', 'g');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('{1}');
addThis();


status = inSection(4);
try
{
  pattern = new RegExp('{1[.!}]*}', 'g');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('{1}');
addThis();


string = 'abccccc{3 }c{ 3}c{3, }c{3 ,}c{3 ,4}c{3, 4}c{3,4 }de';


status = inSection(5);
try
{
  pattern = new RegExp('c{3}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('ccc');
addThis();


status = inSection(6);
try
{
  pattern = new RegExp('c{3 }');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3 }');
addThis();

status = inSection(7);
try
{
  pattern = new RegExp('c{3.}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3 }');
addThis();

status = inSection(8);
try
{
  
  
  
  pattern = new RegExp('c{3\\s}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3 }');
addThis();

status = inSection(9);
try
{
  pattern = new RegExp('c{3[ ]}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3 }');
addThis();

status = inSection(10);
try
{
  pattern = new RegExp('c{ 3}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{ 3}');
addThis();


status = inSection(11);
try
{
  pattern = new RegExp('c{3,}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('ccccc');
addThis();


status = inSection(12);
try
{
  pattern = new RegExp('c{3, }');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3, }');
addThis();

status = inSection(13);
try
{
  pattern = new RegExp('c{3 ,}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3 ,}');
addThis();


status = inSection(14);
try
{
  pattern = new RegExp('c{3,4}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('cccc');
addThis();


status = inSection(15);
try
{
  pattern = new RegExp('c{3 ,4}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3 ,4}');
addThis();

status = inSection(16);
try
{
  pattern = new RegExp('c{3, 4}');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3, 4}');
addThis();

status = inSection(17);
try
{
  pattern = new RegExp('c{3,4 }');
  actualmatch = string.match(pattern);
}
catch (e)
{
  pattern = 'error';
  actualmatch = '';
}
expectedmatch = Array('c{3,4 }');
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
