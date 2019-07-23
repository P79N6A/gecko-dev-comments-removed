




































var gTestfile = 'regress-421621.js';

var BUGNUMBER = 421621;
var summary = 'Do not assert with setter, export/import: (sprop)->slot != SPROP_INVALID_SLOT || !SPROP_HAS_STUB_SETTER(sprop)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var th = this;
this.__defineSetter__('x', function () {});
export *;
import th.*;
x;

reportCompare(expect, actual, summary);
