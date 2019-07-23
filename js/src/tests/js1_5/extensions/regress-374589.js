




































var gTestfile = 'regress-374589.js';

var BUGNUMBER = 374589;
var summary = 'Do not assert decompiling try { } catch(x if true) { } ' +
  'catch(y) { } finally { this.a.b; }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () {
    try { } catch(x if true) { } catch(y) { } finally { this.a.b; } };

  expect = 'function () { try { } catch(x if true) { } catch(y) { } ' +
    'finally { this.a.b; } }';

  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
