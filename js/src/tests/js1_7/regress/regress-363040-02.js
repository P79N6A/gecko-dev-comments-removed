




































var gTestfile = 'regress-363040-02.js';

var BUGNUMBER = 363040;
var summary = 'Array.prototype.reduce application in array flattening';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function flatten(arr)
  {
    function op(partial, item)
    {
      if (item instanceof Array)
        Array.prototype.push.apply(partial, flatten(item));
      else
        partial.push(item);

      return partial;
    }

    return arr.reduce(op, []);
  }

  expect = [1, 2, 3];
  actual = flatten([1, 2, 3]);      
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  expect = [1, 2, 3];
  actual = flatten([1, [2], 3]);    
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  expect = [2, 3];
  actual = flatten([[], 2, 3]);     
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  expect = [1, 2, 3];
  actual = flatten([[1], 2, 3]);    
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  expect = [4];
  actual = flatten([[[[4]]]]);      
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  expect = [1, 2, 3];
  actual = flatten([1, [2, [3]]]);  
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  expect = [];
  actual = flatten([[[[[]]]]]);     
  reportCompare(expect + '', actual + '', summary + ': ' + expect);

  exitFunc ('test');
}
