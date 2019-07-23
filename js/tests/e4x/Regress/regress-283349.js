




































START("13.3.5.4 - [[GetNamespace]]");

var bug = 283349;
var summary = 'Test assertion adding namespace';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var x = <x>text</x>;
var ns = new Namespace("http://foo.com/bar");
x.addNamespace(ns);
printStatus(x.toXMLString());

actual = 'No Crash';

TEST("13.3.5.4 - [[GetNamespace]]", expect, actual);

END();
