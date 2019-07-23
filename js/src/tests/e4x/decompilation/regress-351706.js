





































gTestfile = 'regress-351706.js';

var BUGNUMBER = 351706;
var summary = 'decompilation of E4X literals with parens';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f;

f = function() { return <{m}/>.(y) }
expect = 'function() { return (<{m}/>).(y); }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

f = function() { return (<{m}/>).(y) }
expect = 'function() { return (<{m}/>).(y); }';
actual = f + '';
compareSource(expect, actual, inSection(2) + summary);

END();
