







var BUGNUMBER = 622167;
var summary = 'Handle infinite recursion';
print(BUGNUMBER + ": " + summary);





function eval() { eval(); }

function DoWhile_3()
{
  eval();
}

try
{
  DoWhile_3();
}
catch(e) { }

var r;
function f()
{
  r = arguments;
  test();
  yield 170;
}

function test()
{
  function foopy()
  {
    try
    {
      for (var i in f());
    }
    catch (e) { }
  }
  foopy();
  gc();
}
test();



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
