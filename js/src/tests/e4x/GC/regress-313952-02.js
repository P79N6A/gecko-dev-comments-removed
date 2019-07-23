




































gTestfile = 'regress-313952-02.js';

var summary = "13.3.5.2 - root QName.uri";
var BUGNUMBER = 313952;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var str = String(1);
var expected = String(1);

var x = new XML("text");

x.function::toString = function() {
        var tmp = str;
        str = null;
        return tmp;
}

var TWO = 2.0;

var likeString = {
        toString: function() {
                var tmp = new XML("");
                tmp = (tmp == "string");
                if (typeof gc == "function")
                        gc();
                for (var i = 0; i != 40000; ++i) {
                        tmp = 1e100 * TWO;
                        tmp = null;
                }
                return expected;
        }
}

TEST(1, true, x == likeString);

END();
