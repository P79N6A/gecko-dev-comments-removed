




































var gTestfile = 'regress-452498-102.js';

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
 




  (function(){function x(){} function x()y})();




  function f() {
    "" + (function(){
        for( ; [function(){}] ; x = 0)
          with({x: ""})
            const x = []
            });
  }
  f();





  try
  {
    function f() {
      var x;
      eval("const x = [];");
    }
    f();
  }
  catch(ex)
  {
  }



  try
  {
    do {x} while([[] for (x in []) ]);
  }
  catch(ex)
  {
  }



  try
  {
    {x} ((x=[] for (x in []))); x;
  }
  catch(ex)
  {
  }





  reportCompare(expect, actual, summary);

  exitFunc ('test');
}



