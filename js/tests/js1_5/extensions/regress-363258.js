




































var gTestfile = 'regress-363258.js';


var BUGNUMBER = 363258;
var summary = 'Timer resolution';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var start = 0;
  var stop  = 0;
  var i;
  var limit = 0;
  var incr  = 10;
  var resolution = 5;
  
  while (stop - start == 0)
  {
    limit += incr;
    start = Date.now();
    for (i = 0; i < limit; i++) {}
    stop = Date.now();
  }

  print('limit=' + limit + ', resolution=' + resolution + ', time=' + (stop - start));

  expect = true;
  actual = (stop - start <= resolution);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
