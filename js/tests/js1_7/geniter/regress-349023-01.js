




































var gTestfile = 'regress-349023-01.js';

var BUGNUMBER = 349023;
var summary = 'Bogus JSCLASS_IS_EXTENDED in the generator class';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function gen() {
    var i = 0;
    yield i;
  }

  try
  {
    var g = gen();
    for (var i = 0; i < 10; i++) {
      print(g.next());
    }
  }
  catch(ex)
  {
  }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
