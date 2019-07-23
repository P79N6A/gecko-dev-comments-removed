





































gTestfile = 'regress-371369.js';

var BUGNUMBER = 371369;
var summary = 'delete xml.function::name does not work';
var actual = 'No Exception';
var expect = 'No Exception';

printBugNumber(BUGNUMBER);
START(summary);

var xml = <a/>;
xml.function::something = function() { };

delete xml.function::something;

try {
    xml.something();
    throw "Function can be called after delete";
} catch(e) {
    if (!(e instanceof TypeError))
        throw "Unexpected exception: " + e;
}

TEST(1, expect, actual);
END();
