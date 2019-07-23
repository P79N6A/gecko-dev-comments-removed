




































var gTestfile = 'regress-475645-02.js';

var BUGNUMBER = 475645;
var summary = 'Do not crash @ nanojit::LIns::isop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  if (typeof window != 'undefined')
  {
    var q = (function () { });
    window.addEventListener("load", q, false);
    window.onerror = q;
    arr = new Array();
    pic = r = new Array;
    h = t = 7;
    var pics = "";
    pic[2] = "";
    for (i=1; i < pic.length; i++) 
    {
      try
      {
        if(pics=="")
          pics=pic[i];
        else
          (pic[i]-1 & pic[i].i("") == 1);
      }
      catch(ex)
      {
      }
      arr[i]='';
    }
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
