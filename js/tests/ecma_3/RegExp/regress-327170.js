




































var bug = 327170;
var summary = 'Reuse of RegExp in string.replace(rx.compile(...), function() { rx.compile(...); }) causes a crash';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var g_rx = /(?:)/;

"this is a test-string".replace(g_rx.compile("test", "g"), 
function()
{
  
  
  g_rx.compile("string", "g");
});
  
reportCompare(expect, actual, summary);
