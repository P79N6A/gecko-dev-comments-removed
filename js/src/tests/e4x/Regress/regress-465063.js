





































gTestfile = 'regress-465063.js';

var summary = 'Do not crash @ TraceRecorder::hasMethod';
var BUGNUMBER = 465063;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

jit(true);

y = <x/>;

for (var z = 0; z < 2; ++z) { [] + y; };

jit(false);

TEST(1, expect, actual);

END();
