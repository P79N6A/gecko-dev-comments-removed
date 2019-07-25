




assertEq(raisesException(TypeError)('String.prototype.toString.call(42)'), true);
assertEq(raisesException(TypeError)('String.prototype.toString.call(true)'), true);
assertEq(raisesException(TypeError)('String.prototype.toString.call({})'), true);
assertEq(raisesException(TypeError)('String.prototype.toString.call(null)'), true);
assertEq(raisesException(TypeError)('String.prototype.toString.call([])'), true);
assertEq(raisesException(TypeError)('String.prototype.toString.call(undefined)'), true);
assertEq(completesNormally('String.prototype.toString.call("")'), true);

reportCompare(true, true);
