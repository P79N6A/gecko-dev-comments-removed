






function arr() {
  return Object.defineProperty([1, 2, 3, 4], 2, {configurable: false});
}

assertEq(testLenientAndStrict('var a = arr(); a.length = 2; a',
                              returnsCopyOf([1, 2, 3]),
                              raisesException(TypeError)),
         true);




function addx(obj) {
  obj.x = 5;
  return obj;
}

assertEq(testLenientAndStrict('var a = addx(arr()); a.length = 2; a',
                              returnsCopyOf(addx([1, 2, 3])),
                              raisesException(TypeError)),
         true);

reportCompare(true, true);
