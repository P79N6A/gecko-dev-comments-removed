





var BUGNUMBER = 353116;
var summary = 'Improve errors messages for null, undefined properties';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'TypeError: undefined has no properties';
  actual = 'No Error';

  try
  {
    undefined.y;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  expect = 'TypeError: null has no properties';
  actual = 'No Error';

  try
  {
    null.y;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  expect = 'TypeError: x is undefined';
  actual = 'No Error';

  try
  {
    x = undefined; 
    x.y;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  expect = 'TypeError: x is null';
  actual = 'No Error';

  try
  {
    x = null; 
    x.y;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
