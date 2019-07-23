



















































var gTestfile = 'regress-68498-003.js';
var BUGNUMBER = 68498;
var summary = 'Testing calling obj.eval(str)';
var statprefix = '; currently at expect[';
var statsuffix = '] within test -';
var actual = [ ];
var expect = [ ];



var self = this;


function f(s) {self.eval(s); return y;}



actual[0] = f('var y = 43');
actual[1] = 'y' in self && y;
actual[2] = delete y;
actual[3] = 'y' in self;


expect[0] = 43;
expect[1] = 43;
expect[2] = true;
expect[3] = false;



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
