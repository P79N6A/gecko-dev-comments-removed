







var BUGNUMBER = 858381;
var summary =
  "Array length setting/truncating with non-dense, indexed elements";

print(BUGNUMBER + ": " + summary);





function testTruncateDenseAndSparse()
{
  var arr;

  
  arr = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];

  
  arr[987654321] = 987654321;

  
  arr.length = 8;

  assertEq(987654321 in arr, false);
  assertEq(arr[987654321], undefined);
  assertEq(arr.length, 8);
}
testTruncateDenseAndSparse();

function testTruncateSparse()
{
  
  var arr = [0, 1, 2, 3, 4, 5, 6, 7];

  
  arr[987654321] = 987654321;

  
  arr.length = 8;

  assertEq(987654321 in arr, false);
  assertEq(arr[987654321], undefined);
  assertEq(arr.length, 8);
}
testTruncateSparse();

function testTruncateDenseAndSparseShrinkCapacity()
{
  
  var arr = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

  
  arr[987654321] = 987654321;

  
  arr.length = 8;

  assertEq(987654321 in arr, false);
  assertEq(arr[987654321], undefined);
  assertEq(arr.length, 8);
}
testTruncateDenseAndSparseShrinkCapacity();

function testTruncateSparseShrinkCapacity()
{
  
  var arr = [0, 1, 2, 3, 4, 5, 6, 7];

  
  
  arr[15] = 15;

  
  delete arr[15];

  
  arr[987654321] = 987654321;

  
  arr.length = 8;

  assertEq(987654321 in arr, false);
  assertEq(arr[987654321], undefined);
  assertEq(arr.length, 8);
}
testTruncateSparseShrinkCapacity();



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
