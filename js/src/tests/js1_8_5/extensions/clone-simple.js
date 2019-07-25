



function testEq(b) {
    var a = deserialize(serialize(b));
    assertEq(a, b);
}

testEq(void 0);
testEq(null);

testEq(true);
testEq(false);

testEq(0);
testEq(-0);
testEq(1/0);
testEq(-1/0);
testEq(0/0);
testEq(Math.PI);

testEq("");
testEq("\0");
testEq("a");  
testEq("ab");  
testEq("abc\0123\r\n");  
testEq("\xff\x7f\u7fff\uffff\ufeff\ufffe");  
testEq("\ud800 \udbff \udc00 \udfff"); 
testEq(Array(1024).join(Array(1024).join("x")));  

reportCompare(0, 0, 'ok');
