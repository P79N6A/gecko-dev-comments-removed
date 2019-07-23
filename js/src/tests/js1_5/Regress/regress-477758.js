




































var gTestfile = 'regress-477758.js';

var BUGNUMBER = 477758;
var summary = 'TM: RegExp source';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  function map(array, func) {
    var result = [];
    for(var i=0;i<array.length;i++) {
      result.push(func(array[i]));
    }
    return result;
  }

  function run() {
    var patterns = [/foo/, /bar/];
    function getSource(r) { return r.source; }
    var patternStrings = map(patterns, getSource);
    print(actual += [patterns[0].source, patternStrings[0]] + '');
  }

  expect = 'foo,foo';

  for (var i = 0; i < 4; i++)
  {
    actual = '';
    run();
    reportCompare(expect, actual, summary + ': ' + i);
  }

  jit(false);

  exitFunc ('test');
}
