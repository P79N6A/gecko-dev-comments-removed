




































gTestfile = 'regress-283349.js';

var summary = "13.3.5.4 - [[GetNamespace]]";
var BUGNUMBER = 283349;
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var x = <x>text</x>;
var ns = new Namespace("http://foo.com/bar");
x.addNamespace(ns);
printStatus(x.toXMLString());

actual = 'No Crash';

TEST("13.3.5.4 - [[GetNamespace]]", expect, actual);

END();
