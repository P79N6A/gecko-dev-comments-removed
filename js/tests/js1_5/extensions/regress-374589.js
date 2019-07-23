




































var bug = 374589;
var summary = 'Do not assert decompiling try { } catch(x if true) { } ' + 
  'catch(y) { } finally { this.a.b; }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function () { 
    try { } catch(x if true) { } catch(y) { } finally { this.a.b; } };

  expect = 'function () { try { } catch(x if true) { } catch(y) { } ' + 
    'finally { this.a.b; } }';

  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
