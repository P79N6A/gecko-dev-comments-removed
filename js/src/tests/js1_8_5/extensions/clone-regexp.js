




function testRegExp(b, c=b) {
    var a = deserialize(serialize(b));
    assertEq(a === b, false);
    assertEq(Object.getPrototypeOf(a), RegExp.prototype);
    assertEq(Object.prototype.toString.call(a), "[object RegExp]");
    for (p in a)
        throw new Error("cloned RegExp should have no enumerable properties");

    assertEq(a.source, c.source);
    assertEq(a.global, c.global);
    assertEq(a.ignoreCase, c.ignoreCase);
    assertEq(a.multiline, c.multiline);
    assertEq(a.sticky, c.sticky);
    assertEq("expando" in a, false);
}

testRegExp(RegExp(""));
testRegExp(/(?:)/);
testRegExp(/^(.*)$/gimy);
testRegExp(RegExp.prototype);

var re = /\bx\b/gi;
re.expando = true;
testRegExp(re);




re.__proto__ = {};
testRegExp(re, /\bx\b/gi);

reportCompare(0, 0, 'ok');
