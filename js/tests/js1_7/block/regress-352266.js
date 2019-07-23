




































var bug = 352266;
var summary = 'decompilation of excess indendation should not cause round-trip change';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { if(x) {{ let x = 4; }}  }
  expect = 'function() { if(x) {{ let x = 4; }}  }';
  actual = f +'';
  compareSource(expect, actual, summary);


  f = function () {
    if (x) {

            let x = 4;
    }
}
  expect = 'function () { if (x) { let x = 4; }}';
  actual = f +'';
  compareSource(expect, actual, summary);

  f = function () { if (x) { let x = 4; }}
  expect = 'function () { if (x) { let x = 4; }}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
