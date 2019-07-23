




































var bug = 349482;
var summary = 'Decompiling try/catch in for..in should not crash';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function() { for(p in {})   try{}catch(e){}   };
  print(f.toString());

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
