




































var gTestfile = 'regress-392308.js';

var BUGNUMBER = 392308;
var summary = 'StopIteration should be catchable';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function testStop() {
    function yielder() {
      actual += 'before, ';
      yield;
      actual += 'after, ';
    }

    expect = 'before, after, iteration terminated normally';

    try {
      var gen = yielder();
      result = gen.next();
      gen.send(result);
    } catch (x if x instanceof StopIteration) {
      actual += 'iteration terminated normally';
    } catch (x2) {
      actual += 'unexpected throw: ' + x2;
    }
  }
  testStop();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
