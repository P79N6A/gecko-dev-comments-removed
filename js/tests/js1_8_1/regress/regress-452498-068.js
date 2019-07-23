




































var gTestfile = 'regress-452498-068.js';

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



  try
  {
    eval("*;", (3/0 ? function(id) { return id } : <><x><y/></x></>));
  }
  catch(ex)
  {
  }



  foo = "" + new Function("while(\u3056){let \u3056 = x}");



  function a(){ let c; eval("let c, y"); }
  a();



  try
  {
    {x: 1e+81 ? c : arguments}
  }
  catch(ex)
  {
  }



  (function(q){return q;} for each (\u3056 in []))



    try
    {
      for(let x = <{x}/> in <y/>) x2;
    }
    catch(ex)
    {
    }



  for(
    const NaN;
    this.__defineSetter__("x4", function(){});
    (eval("", (p={})))) let ({} = (((x ))(function ([]) {})), x1) y;



  function f(){ var c; eval("{var c = NaN, c;}"); }
  f();


  try
  {
    eval(
      '  x\n' +
      '    let(x) {\n' +
      '    var x\n'
      );
  }
  catch(ex)
  {
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
