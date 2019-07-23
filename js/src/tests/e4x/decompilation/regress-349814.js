





































gTestfile = 'regress-349814.js';

var BUGNUMBER = 349814;
var summary = 'decompilation of e4x literals';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var z = function ()
{
  a =
    <x>
      <y/>
    </x>;
};

expect = z + '';
actual = (eval("(" + z + ")")) + '';

compareSource(expect, actual, inSection(1) + summary);

END();
