




































var gTestfile = 'regress-424311.js';

var BUGNUMBER = 424311;
var summary = 'Do not assert: entry->kpc == ((PCVCAP_TAG(entry->vcap) > 1) ? (jsbytecode *) JSID_TO_ATOM(id) : cx->fp->regs->pc)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function(){(function(){ constructor=({}); })()})();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
