




assertEq(raisesException(TypeError)('String.prototype.toSource.call(42)'), true);
assertEq(raisesException(TypeError)('String.prototype.toSource.call(true)'), true);
assertEq(raisesException(TypeError)('String.prototype.toSource.call({})'), true);
assertEq(raisesException(TypeError)('String.prototype.toSource.call(null)'), true);
assertEq(raisesException(TypeError)('String.prototype.toSource.call([])'), true);
assertEq(raisesException(TypeError)('String.prototype.toSource.call(undefined)'), true);
assertEq(completesNormally('String.prototype.toSource.call("")'), true);

reportCompare(true, true);
