




































START("13.3.5.2 - QName.uri");

var bug = 313952;
var summary = 'Root QName.uri';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

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
