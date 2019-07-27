







var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);






  for (var [x] in null) ;



  for (var f in null)
    ;
  var f = 1;
  (f)




    try { eval('let (x = 1) { var x; }'); } catch(ex) {}



  (1 for each (x in x));

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
