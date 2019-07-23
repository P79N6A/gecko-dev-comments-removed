




































var gTestfile = 'regress-452498-135.js';

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






  for (let i = 0; i < 9; ++i) {
    let m = i;
    if (i % 3 == 1) {
      print('' + (function() { return m; })());
    }
  }





  try
  {
    (x for each (c in []))
      x
      }
  catch(ex)
  {
  }




  "" + (function(){L:if (*::*){ var x } function x(){}})



    "" + (function(){if (*::*){ var x } function x(){}})



    "" + (function(){{<y/>; throw <z/>;for(var x = [] in false) return } function x(){}})



    try
    {
      (function(){for(; (this); ((window for (x in [])) for (y in []))) 0});
    }
    catch(ex)
    {
    }


  eval(uneval( function(){
        ((function()y)() for each (x in this))
          } ))





    for (let a=0;a<3;++a) for (let b=0;b<3;++b) if ((g=a|(a%b))) with({}){}



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
