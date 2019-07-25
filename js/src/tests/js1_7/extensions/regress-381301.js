






































var BUGNUMBER = 381301;
var summary = 'uneval of object with native-function getter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var o =
    Object.defineProperty({}, "x", { get: decodeURI, enumerable: true, configurable: true });
  expect = '( { get x ( ) { [ native code ] } } )';
  actual =  uneval(o);

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
