






































var gTestfile = 'regress-363988.js';

var BUGNUMBER = 363988;
var summary = 'Do not crash at JS_GetPrivate with large script';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function crash() {
    var town = new Array;

    for (var i = 0; i < 0x4001; ++i) {
      var si = String(i);
      town[i] = [ si, "x" + si, "y" + si, "z" + si ];
    }

    return "town=" + uneval(town) + ";function f() {}";
  }

  if (typeof document != "undefined")
  {
    
    document.write("<script>", crash(), "<\/script>");
  }
  else
  {
    crash();
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
