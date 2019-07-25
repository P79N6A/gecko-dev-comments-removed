






































var BUGNUMBER = 379566;
var summary = 'Keywords after get|set';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '({ ' +
    'get in () { return this.for; }, ' + 
    'set in (value) { this.for = value; } ' + 
    '})';
  try
  {
    var obj = eval('({ ' +
                   'get in() { return this.for; }, ' + 
                   'set in(value) { this.for = value; } ' + 
                   '})');
    actual = obj.toSource();

  }
  catch(ex)
  {
    actual = ex + '';
  }

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
