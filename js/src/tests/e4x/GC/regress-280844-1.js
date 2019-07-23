





































gTestfile = 'regress-280844-1.js';

var summary = 'Uncontrolled recursion in js_MarkXML during GC';
var BUGNUMBER = 280844;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var N = 5 * 1000;
var x = <x/>;
for (var i = 1; i <= N; ++i) {
    x.appendChild(<x/>);
    x = x.x[0];
}
printStatus(x.toXMLString());
gc();

TEST(1, expect, actual);
END();
