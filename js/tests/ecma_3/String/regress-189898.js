













































var gTestfile = 'regress-189898.js';
var UBound = 0;
var BUGNUMBER = 189898;
var summary = 'Regression test for bug 189898';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = 'XaXY'.replace('XY', '--')
  expect = 'Xa--';
addThis();

status = inSection(2);
actual = '$a$^'.replace('$^', '--')
  expect = '$a--';
addThis();

status = inSection(3);
actual = 'ababc'.replace('abc', '--')
  expect = 'ab--';
addThis();

status = inSection(4);
actual = 'ababc'.replace('abc', '^$')
  expect = 'ab^$';
addThis();










status = inSection(5);
actual = 'XaXY'.replace(/XY/, '--')
  expect = 'Xa--';
addThis();

status = inSection(6);
actual = 'XaXY'.replace(/XY/g, '--')
  expect = 'Xa--';
addThis();

status = inSection(7);
actual = '$a$^'.replace(/\$\^/, '--')
  expect = '$a--';
addThis();

status = inSection(8);
actual = '$a$^'.replace(/\$\^/g, '--')
  expect = '$a--';
addThis();

status = inSection(9);
actual = 'ababc'.replace(/abc/, '--')
  expect = 'ab--';
addThis();

status = inSection(10);
actual = 'ababc'.replace(/abc/g, '--')
  expect = 'ab--';
addThis();

status = inSection(11);
actual = 'ababc'.replace(/abc/, '^$')
  expect = 'ab^$';
addThis();

status = inSection(12);
actual = 'ababc'.replace(/abc/g, '^$')
  expect = 'ab^$';
addThis();




test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
