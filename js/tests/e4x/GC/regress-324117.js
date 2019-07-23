





































gTestfile = 'regress-324117.js';

var summary = "GC hazard during namespace scanning";
var BUGNUMBER = 324117;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

function prepare(N)
{
    var xml = <xml/>;
    var ns1 = new Namespace("text1"); 
    var ns2 = new Namespace("text2"); 
    xml.addNamespace(ns1);
    xml.addNamespace(ns2);

    
    cursor = xml;
    for (var i = 0; i != N; ++i) {
        if (i % 2 == 0)
            cursor = [ {a: 1}, cursor ];
        else
            cursor = [ cursor, {a: 1} ];
    }
    return cursor;
}

function check(list, N)
{
    
    for (var i = N; i != 0; --i) {
        list = list[i % 2];
    }
    var xml = list;
    if (typeof xml != "xml")
        return false;
    var array = xml.inScopeNamespaces();
    if (array.length !== 3)
        return false;
    if (array[0].uri !== "")
        return false;
    if (array[1].uri !== "text1")
        return false;
    if (array[2].uri !== "text2")
        return false;

    return true;
}

var N = 64000;
var list = prepare(N);
gc();
var ok = check(list, N);
printStatus(ok);

TEST(1, expect, actual);

END();
