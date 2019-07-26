load(libdir + "eqArrayHelper.js");

assertEqArray([...[1, 2, 3]], [1, 2, 3]);
assertEqArray([1, ...[2, 3, 4], 5], [1, 2, 3, 4, 5]);
assertEqArray([1, ...[], 2], [1, 2]);
assertEqArray([1, ...[2, 3], 4, ...[5, 6]], [1, 2, 3, 4, 5, 6]);
assertEqArray([1, ...[], 2], [1, 2]);
assertEqArray([1,, ...[2]], [1,, 2]);
assertEqArray([1,, ...[2],, 3,, 4,], [1,, 2,, 3,, 4,]);
assertEqArray([...[1, 2, 3],,,,], [1, 2, 3,,,,]);
assertEqArray([,,...[1, 2, 3],,,,], [,,1,2,3,,,,]);






