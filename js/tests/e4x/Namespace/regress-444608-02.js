





































gTestfile = 'regress-444608-02.js';

var summary = '13.2 Namespaces - call constructors directly';
var BUGNUMBER = 444608;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var x = <xml/>;
Namespace = function() { return 10; };
x.removeNamespace("x");

TEST(1, expect, actual);

END();
