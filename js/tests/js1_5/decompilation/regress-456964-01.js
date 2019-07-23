




































var gTestfile = 'regress-456964-01.js';

var BUGNUMBER = 456964;
var summary = 'Infinite loop decompling function';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  
  function Test()
  {
    var object = { abc: 1, def: 2};
    var o = '';
    for (var i in object)
      o += i + ' = ' + object[i] + '\n';
    return o;
  }

  print(Test);

  expect = 'function Test ( ) { var object = { abc : 1 , def : 2 } ; var o = ""; for ( var i in object ) { o += i + " = " + object [ i ] + "\\ n "; } return o ; }';
  actual = Test + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
