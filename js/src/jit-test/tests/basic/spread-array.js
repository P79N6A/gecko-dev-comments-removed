load(libdir + "asserts.js");
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

assertEqArray([...[undefined]], [undefined]);


assertEqArray([...Int32Array([1, 2, 3])], [1, 2, 3]);
assertEqArray([..."abc"], ["a", "b", "c"]);
assertEqArray([...[1, 2, 3].iterator()], [1, 2, 3]);
assertEqArray([...Set([1, 2, 3])], [1, 2, 3]);
assertEqArray([...Map([["a", "A"], ["b", "B"], ["c", "C"]])].map(([k, v]) => k + v), ["aA", "bB", "cC"]);
let itr = {
  iterator: function() {
    return {
      i: 1,
      next: function() {
        if (this.i < 4)
          return this.i++;
        else
          throw StopIteration;
      }
    };
  }
};
assertEqArray([...itr], [1, 2, 3]);
let gen = {
  iterator: function() {
    for (let i = 1; i < 4; i ++)
      yield i;
  }
};
assertEqArray([...gen], [1, 2, 3]);

let a, b = [1, 2, 3];
assertEqArray([...a=b], [1, 2, 3]);






assertThrowsInstanceOf(() => [...null], TypeError);
assertThrowsInstanceOf(() => [...undefined], TypeError);

