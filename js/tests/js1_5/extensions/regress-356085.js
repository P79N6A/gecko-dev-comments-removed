




































var bug = 356085;
var summary = 'js_obj_toString for getter/setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = '({ set p y() { } })';
  actual = uneval({p setter: function y() { } });

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
