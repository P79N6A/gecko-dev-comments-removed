





































var BUGNUMBER = 350256;
var summary = 'Array.apply maximum arguments: 2^19-1024';
var actual = '';
var expect = '';

expectExitCode(0);
expectExitCode(5);


test(Math.pow(2, 19)-1024);


function test(length)
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var a = new Array();
  a[length - 2] = 'length-2';
  a[length - 1] = 'length-1';

  var b = Array.apply(null, a);

  expect = length + ',length-2,length-1';
  actual = b.length + "," + b[length - 2] + "," + b[length - 1];
  reportCompare(expect, actual, summary);

  function f() {
    return arguments.length + "," + arguments[length - 2] + "," +
      arguments[length - 1];
  }

  expect = length + ',length-2,length-1';
  actual = f.apply(null, a);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
