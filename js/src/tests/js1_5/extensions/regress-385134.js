





var BUGNUMBER = 385134;
var summary = 'Do not crash with setter, watch, uneval';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof this.__defineSetter__ != 'undefined' && 
      typeof this.watch != 'undefined' &&
      typeof uneval != 'undefined')
  {
    try {
      this.__defineSetter__(0, function(){});
    } catch (exc) {
      
    }
    this.watch(0, function(){});
    uneval(this);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
