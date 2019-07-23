




































var gTestfile = 'regress-419018.js';

var BUGNUMBER = 419018;
var summary = 'UMR in JSENUMERATE_INIT';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('This test must be run under valgrind to check if an UMR occurs in slowarray_enumerate');

  try
  {
    function parse() {
      var a = []; 
      a["b"] = 1; 
      return a;
    }
    
    
    
    var x = parse(""); 
    
    for (var o in x)
      c[o]; 
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

