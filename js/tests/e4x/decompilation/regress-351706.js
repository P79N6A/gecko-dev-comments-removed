





































var bug = 351706;
var summary = 'decompilation of E4X literals with parens';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f;

f = function() { return <{m}/>.(y) }
expect = 'function() { return (<{m}/>).(y); }';
actual = f + '';
compareSource(1, expect, actual);

f = function() { return (<{m}/>).(y) }
expect = 'function() { return (<{m}/>).(y); }';
actual = f + '';
compareSource(2, expect, actual);

END();
