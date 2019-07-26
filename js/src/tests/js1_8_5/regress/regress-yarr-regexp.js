function toSource(o) { return o === null ? "null" : o.toSource(); }

var gcgcz = /((?:.)+)((?:.)*)/; 
reportCompare(["a", "a", ""].toSource(), gcgcz.exec("a").toSource());
reportCompare(["ab", "ab", ""].toSource(), gcgcz.exec("ab").toSource());
reportCompare(["abc", "abc", ""].toSource(), gcgcz.exec("abc").toSource());

reportCompare(["a", ""].toSource(), toSource(/((?:)*?)a/.exec("a")));
reportCompare(["a", ""].toSource(), toSource(/((?:.)*?)a/.exec("a")));
reportCompare(["a", ""].toSource(), toSource(/a((?:.)*)/.exec("a")));

reportCompare(["B", "B"].toSource(), toSource(/([A-Z])/.exec("fooBar")));


reportCompare(/x{2147483648}x/.test('1'), false);
reportCompare(/x{2147483648,}x/.test('1'), false);
reportCompare(/x{2147483647,2147483648}x/.test('1'), false);
