03




































var gTestfile = 'regress-452498-117.js';

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
    eval('x; function  x(){}; const x;');
  }
  catch(ex)
  {
  }



  (function(){ var x; eval("var x; x = null"); })()



    function this ({x}) { function x(){} }



  for(let x = [ "" for (y in /x/g ) if (x)] in (""));



  (function(){const x = 0, y = delete x;})()



    try
    {
      (function(){(yield []) (function(){with({}){x} }); const x;})();
    }
    catch(ex)
    {
    }



  try
  {
    (function(){([]) ((function(q) { return q; })for (each in *))})();
  }
  catch(ex)
  {
  }




  try
  {
    eval("((x1) > [(x)(function() { x;}) for each (x in x)])()");
  }
  catch(ex)
  {
  }



  uneval(function(){for(var [arguments] = ({ get y(){} }) in y ) (x);});



  uneval(function(){([] for ([,,]in <><y/></>));});



  try
  {
    eval('(function(){{for(c in (function (){ for(x in (x1))window} )()) {const x;} }})();');
  }
  catch(ex)
  {
  }



  try
  {
    (eval("(function(){let x , x =  (x for (x in null))});"))();
  }
  catch(ex)
  {
  }




  "" + function(){for(var [x] in x1) ([]); function x(){}}




  try
  {
    eval(
      "for (a in (function(){" +
      "      for each (let x in ['']) { return new function x1 (\u3056) { yield x } ();" +
      "        }})())" +
      "  function(){}"
      );
  }
  catch(ex)
  {
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
