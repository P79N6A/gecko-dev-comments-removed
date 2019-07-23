




































var gTestfile = 'regress-416834.js';

var BUGNUMBER = 416834;
var summary = 'Do not assert: !entry || entry->kpc == ((PCVCAP_TAG(entry->vcap) > 1) ? (jsbytecode *) JSID_TO_ATOM(id) : cx->fp->pc)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
this.__proto__.x = eval;
for (i = 0; i < 16; ++i) 
  delete eval;
(function w() { x = 1; })();

reportCompare(expect, actual, summary);
