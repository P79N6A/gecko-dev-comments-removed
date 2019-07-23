




































var gTestfile = 'regress-465453.js';

var BUGNUMBER = 465453;
var summary = 'Do not convert (undefined) to "undefined"';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '[(new Boolean(true)), (void 0), (new Boolean(true)), ' + 
    '(new Boolean(true)), (void 0), (void 0), "", "", (void 0)]';

  jit(true);
  var out = [];
  for each (var e in [(new Boolean(true)), 
                      (void 0), 
                      (new Boolean(true)), 
                      (new Boolean(true)), 
                      (void 0), 
                      (void 0), 
                      "", 
                      "", 
                      (void 0)])
             out.push(e);
  print(actual = uneval(out));
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
