





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



  function m()
  {
    function a() { }
    function b() { a(); }
    this.c = function () { b(); }
  }
  (new m).c();


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
