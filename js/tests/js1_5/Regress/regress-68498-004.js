























































var gTestfile = 'regress-68498-004.js';
var BUGNUMBER = 68498;
var summary = 'Testing self.eval(str) inside a function';
var statprefix = '; currently at expect[';
var statsuffix = '] within test -';
var sToEval='';
var actual=[ ];
var expect=[ ];



var self = this;


var x = 'outer';


function f(o,s,x) {with(o) eval(s); return z;};


sToEval += 'actual[0] = typeof g;'
sToEval += 'function g(){actual[1]=(typeof w == "undefined"  ||  w); return x};'
sToEval += 'actual[2] = w;'
sToEval += 'actual[3] = typeof g;'
sToEval += 'var z=g();'


actual[4] = f({w:44}, sToEval, 'inner');
actual[5] = 'z' in self && z;















expect[0] = 'function';
expect[1] = 44;
expect[2] = 44;
expect[3] = 'function';
expect[4] = 'inner';
expect[5] = false;




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
