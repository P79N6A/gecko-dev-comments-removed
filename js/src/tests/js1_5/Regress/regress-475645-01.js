




































var gTestfile = 'regress-475645-01.js';

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

  linkarr = new Array();
  picarr = new Array();
  textarr = new Array();
  var f=161;
  var t=27;
  var pics = "";
  var links = "";
  var texts = "";
  var s = f+t;
  var d = "1";
  picarr[2] = "2";
  for(i=1;i<picarr.length;i++)
  {
    if(pics=="") pics = picarr[i];
    else{
      if(picarr[i].indexOf("jpg")==-1 && picarr[i].indexOf("JPG")==-1) picarr[i] = d;
    }
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
