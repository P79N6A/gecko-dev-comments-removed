





































gTestfile = 'regress-353165.js';

var BUGNUMBER = 353165;
var summary = 'Do not crash with xml_getMethod';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

crash1();
crash2();

END();

function crash1()
{
    try {
        var set = new XML('<a><b>text</b></a>').children('b');
        var counter = 0;
        Object.prototype.unrooter getter = function() {
            ++counter;
            if (counter == 5) {
                set[0] = new XML('<c/>');
                if (typeof gc == "function") {
                    gc();
                    var tmp = Math.sqrt(2), tmp2;
                    for (i = 0; i != 50000; ++i)
                        tmp2 = tmp / 2;
                }
            }
            return undefined;
        }

        set.unrooter();
    }
    catch(ex) {
        print('1: ' + ex);
    }
    TEST(1, expect, actual);

}

function crash2() {
    try {
        var expected = "SOME RANDOM TEXT";

        var set = <a><b>{expected}</b></a>.children('b');
        var counter = 0;

        function unrooter_impl() {
                return String(this);
        }

        Object.prototype.unrooter getter = function() {
            ++counter;
            if (counter == 7)
            return unrooter_impl;
            if (counter == 5) {
                set[0] = new XML('<c/>');
                if (typeof gc == "function") {
                    gc();
                    var tmp = 1e500, tmp2;
                    for (i = 0; i != 50000; ++i)
                        tmp2 = tmp / 1.1;
                }
            }
            return undefined;
        }

        set.unrooter();
    }
    catch(ex) {
        print('2: ' + ex);
    }
    TEST(2, expect, actual);
}
