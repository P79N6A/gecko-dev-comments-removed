





































gTestfile = 'regress-462734-01.js';

var summary = 'Do not assert: pobj_ == obj2';
var BUGNUMBER = 462734;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var save__proto__ = this.__proto__;

try
{
    for (x in function(){}) (<x><y/></x>);
    this.__defineGetter__("x", Function);
    __proto__ = x;
    prototype += <x><y/></x>;
}
catch(ex)
{
    print(ex + '');
}

__proto__ = save__proto__;

TEST(1, expect, actual);

END();
