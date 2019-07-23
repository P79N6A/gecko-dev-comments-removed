





































var bug = 346090;
var summary = 'Do not crash with this regexp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var r = /%((h[^l]+)|(l[^h]+)){0,2}?a/g;
  r.exec('%lld %d');

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
