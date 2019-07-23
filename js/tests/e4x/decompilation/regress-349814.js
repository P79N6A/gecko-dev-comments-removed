





































var bug = 349814;
var summary = 'decompilation of e4x literals';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var z = function ()
{ 
  a =
    <x>
      <y/>
    </x>;
};

expect = z + '';
actual = (eval("(" + z + ")")) + '';

compareSource(1, expect, actual);

END();
