





































gTestfile = 'regress-366123.js';

var BUGNUMBER = 366123;
var summary = 'Compiling long XML filtering predicate';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

function exploit() {
  var code = "foo = <x/>.(", obj = {};
  for(var i = 0; i < 0x10000; i++) {
    code += "0, ";
  }
  code += "0);";
  Function(code);
}
try
{
    exploit();
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
