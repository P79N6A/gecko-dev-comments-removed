





































var gTestfile = 'regress-301574.js';

var BUGNUMBER = 301574;
var summary = 'E4X should be enabled even when e4x=1 not specified';
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  var xml = XML('<xml/>');
}
catch(e)
{
  actual = 'error: ' + e;
} 

reportCompare(expect, actual, summary + ': XML()');

try
{
  var xml = XMLList('<p>text</p>');
}
catch(e)
{
  actual = 'error: ' + e;
} 

reportCompare(expect, actual, summary + ': XMLList()');
