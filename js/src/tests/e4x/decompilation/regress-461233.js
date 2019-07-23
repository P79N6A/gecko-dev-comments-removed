





































gTestfile = 'regress-461233.js';

var summary = 'Decompilation of ({0: (4, <></>)})';
var BUGNUMBER = 461233;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = (function() { return ({0: (4, <></>) }); });

expect = 'function() { return {0: (4, <></>) }; }';
actual = f + '';

compareSource(expect, actual, expect)

END();
