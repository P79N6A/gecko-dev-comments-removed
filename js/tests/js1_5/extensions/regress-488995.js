




































var gTestfile = 'regress-488995.js';

var BUGNUMBER = 488995;
var summary = 'Do not crash with watch, __defineSetter__ on svg';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof document == 'undefined')
  {
    print('Test skipped: requires browser.');
  }
  else
  {
    try
    {
      var o=document.createElementNS("http://www.w3.org/2000/svg", "svg");
      var p=o.y;
      p.watch('animVal', function(id, oldvar, newval) {} );
      p.type='xxx';
      p.__defineSetter__('animVal', function() {});
      p.animVal=0;
    }
    catch(ex)
    {
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
