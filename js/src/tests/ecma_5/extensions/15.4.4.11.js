






function arr() {
  return Object.defineProperty([20, 10, 30], 0, {writable: false});
}

assertEq(testLenientAndStrict('var a = arr(); a.sort()',
                              raisesException(TypeError),
                              raisesException(TypeError)),
         true);

function obj() {
  var o = {0: 20, 1: 10, 2: 30, length: 3};
  Object.defineProperty(o, 0, {writable: false});
  return o;
}

assertEq(testLenientAndStrict('var o = obj(); Array.prototype.sort.call(o)',
                              raisesException(TypeError), 
                              raisesException(TypeError)),
         true);





reportCompare(true, true);
