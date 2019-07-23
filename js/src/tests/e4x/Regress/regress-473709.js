





































gTestfile = 'regress-473709.js';

var summary = 'Do not assert: cursor == (uint8 *)copy->messageArgs[0] + argsCopySize';
var BUGNUMBER = 473709;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

function f() { eval("(function() { switch(x, x) { default: for(x2; <x><y/></x>;) (function(){}) <x><y/></x>;break; case (+<><x><y/></x></>): break;   }; })()"); }

if (typeof gczeal == 'function')
{
    gczeal(2);
}

try
{
    f();
}
catch(ex)
{
}

if (typeof gczeal == 'function')
{
    gczeal(0);
}

TEST(1, expect, actual);

END();
