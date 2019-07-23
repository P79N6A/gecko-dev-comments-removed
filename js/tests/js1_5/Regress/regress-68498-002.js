


















































var gTestfile = 'regress-68498-002.js';
var BUGNUMBER = 68498;
var summary = 'Creating a Deletable local variable using eval';
var statprefix = '; currently at expect[';
var statsuffix = '] within test -';
var actual = [ ];
var expect = [ ];



var self = this;


function f(s) {eval(s); actual[0]=y; return delete y;}



actual[1] = f('var y = 42');
actual[2] = 'y' in self && y;


expect[0] = 42;
expect[1] = true;
expect[2] = false;



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var i in expect)
  {
    reportCompare(expect[i], actual[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return (summary  +  statprefix  +  i  +  statsuffix);
}
