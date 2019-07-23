




































var gTestfile = 'regress-356085.js';

var BUGNUMBER = 356085;
var summary = 'js_obj_toString for getter/setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '({ set p y() { } })';
  actual = uneval({p setter: function y() { } });

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
