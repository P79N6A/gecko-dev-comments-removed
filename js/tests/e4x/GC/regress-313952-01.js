




































gTestfile = 'regress-313952-01.js';

var summary = "13.3.5.2 - root QName.uri";
var BUGNUMBER = 313952;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);
printStatus("This test requires TOO_MUCH_GC");

var str = " foo:bar".substring(1);
expect = new QName("  foo:bar".substring(2), "a").uri;

var likeString = {
        toString: function() {
                var tmp = str;
                str = null;
                return tmp;
        }
};

actual = new QName(likeString, "a").uri;

printStatus(actual.length);

printStatus(expect === actual);

TEST(1, expect, actual);

END();
