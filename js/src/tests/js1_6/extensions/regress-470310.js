





































var BUGNUMBER = 470310;
var summary = 'Do not assert: (uint32_t)((atoms - script->atomMap.vector + ' + 
  '((uintN)(((regs.pc + 0)[1] << 8) | (regs.pc + 0)[2])))) < objects_->length';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'TypeError: 6 is not a function';

  this.__defineSetter__('m', [].map);
  function f() { for (var j = 0; j < 4; ++j) if (j == 3) m = 6; }
  try { f(); } catch(e) { print(actual = e + ''); }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
