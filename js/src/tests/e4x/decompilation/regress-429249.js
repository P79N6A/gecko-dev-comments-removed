







































var summary = 'trap should not change decompilation <x/>';
var BUGNUMBER = 429249
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

function g() {
    return <x/>;
}

expect = 'function g() { return <x/>; }';
actual = g + '';
compareSource(expect, actual, summary + ' : before trap');

if (typeof trap == 'function' && typeof setDebug == 'function')
{
    setDebug(true);
    trap(g, 0, "");
    actual = g + '';
    compareSource(expect, actual, summary + ' : after trap');
}

END();
