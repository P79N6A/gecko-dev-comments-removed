





var BUGNUMBER = 909602;
var summary =
  "Array.prototype.pop shouldn't touch elements greater than length on " +
  "non-arrays";

print(BUGNUMBER + ": " + summary);





function doTest(obj, index)
{
  
  assertEq(Array.prototype.pop.call(obj), undefined);
  assertEq(index in obj, true);
  assertEq(obj[index], 42);
}




function testPop1()
{
  var obj = { length: 2, 3: 42 };
  doTest(obj, 3);
}
for (var i = 0; i < 50; i++)
  testPop1();


function testPop2()
{
  var obj = { length: 0, 3: 42 };
  doTest(obj, 3);
}
for (var i = 0; i < 50; i++)
  testPop2();




function testPop3()
{
  var obj = { length: 2, 55: 42 };
  doTest(obj, 55);
}
for (var i = 0; i < 50; i++)
  testPop3();


function testPop4()
{
  var obj = { length: 0, 55: 42 };
  doTest(obj, 55);
}
for (var i = 0; i < 50; i++)
  testPop4();




function testPop5()
{
  var obj = { length: 2, 65530: 42 };
  doTest(obj, 65530);
}
for (var i = 0; i < 50; i++)
  testPop5();


function testPop6()
{
  var obj = { length: 0, 65530: 42 };
  doTest(obj, 65530);
}
for (var i = 0; i < 50; i++)
  testPop6();



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
