





































gTestfile = 'regress-354151-01.js';

var BUGNUMBER = 354151;
var summary = 'Bad assumptions about Array elements';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

function getter() { return 1; }
function setter(v) { return v; }

var xml = <a xmlns="foo:bar"/>;

Array.prototype.__defineGetter__(0, getter);
Array.prototype.__defineSetter__(0, setter);

xml.namespace();

delete Array.prototype[0];

TEST(1, expect, actual);

END();
