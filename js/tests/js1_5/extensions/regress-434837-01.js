




































var gTestfile = 'regress-434837-01.js';

var BUGNUMBER = 434837;
var summary = '|this| in accessors in prototype chain of array';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    expect = true;
    actual = null;
    x = [ "one", "two" ];
    Array.prototype.__defineGetter__('test1', function() { actual = (this === x); });
    x.test1;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': x.test1');

  try
  {
    expect = false;
    actual = null;
    Array.prototype.__defineGetter__('test2', function() { actual = (this === Array.prototype) });
    x.test2;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': x.test2');

  Array.prototype.__defineGetter__('test3', function() { actual = (this === x) });
  Array.prototype.__defineSetter__('test3', function() { actual = (this === x) });

  try
  {
    expect = true;
    actual = null;
    x.test3;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': x.test3 (1)');

  try
  {
    expect = true;
    actual = null;
    x.test3 = 5;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': x.test3 = 5');

  try
  {
    expect = true;
    actual = null;
    x.test3;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': x.test3 (2)');

  try
  {
    var y = ['a', 'b', 'c', 'd'];
    expect = 4;
    actual = y.__count__;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': y.__count__');

  try
  {
    expect = 0;
    actual = [].__count__;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': [].__count__');

  try
  {
    expect = 1;
    actual = [1].__count__;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': [1].__count__');

  try
  {
    expect = 9;
    actual = [1,2,3,4,5,6,7,8,9].__count__;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': [1].__count__');

  exitFunc ('test');
}
