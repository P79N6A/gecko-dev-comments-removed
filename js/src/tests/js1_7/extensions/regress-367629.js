






































var BUGNUMBER = 367629;
var summary = 'Decompilation of result with native function as getter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var a =
    Object.defineProperty({}, "h",
    {
      get: encodeURI,
      enumerable: true,
      configurable: true
    });

  expect = '({get h() {[native code]}})';
  actual = uneval(a);      

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
