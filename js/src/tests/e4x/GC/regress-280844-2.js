





































gTestfile = 'regress-280844-2.js';

var summary = 'Uncontrolled recursion in js_MarkXML during GC';
var BUGNUMBER = 280844;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var N = 100 * 1000;

function prepare_list(N)
{
    var head = {};
    var cursor = head;
    for (var i = 0; i != N; ++i) {
        var ns = new Namespace(); 
        var x = <xml/>;
        x.addNamespace(ns);
        cursor.next = x;
        cursor = ns;
    }
    return head;
}

var head = prepare_list(N);

gc();

TEST(1, expect, actual);

END();
