




































var gTestfile = 'regress-475144.js';

var BUGNUMBER = 475144;
var summary = 'TM: Do not assert: !JS_ON_TRACE(cx)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

function a() {}
function b() {}
function c() {}
eval("this.__defineGetter__(\"\", function(){ return new Function } )");
[[].some for each (x in this) for each (y in /x/g)];

jit(false);

reportCompare(expect, actual, summary);
