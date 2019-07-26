function testSearchFlat() {
    var s1 = toLatin1("fooBar12345");
    var s2 = toLatin1("Bar1");

    
    assertEq(s1.search(s2), 3);
    assertEq(s2.search(s1), -1);
    assertEq(s1.search(s1), 0);

    
    assertEq(s1.search(s2 + "\u1200"), -1);
    assertEq(s1.search(("12345\u1200").slice(0, -1)), 6);

    
    assertEq("fooBar12345\u1200".search(s1), 0);
    assertEq("fooBar12345\u1200".search(s2), 3);

    
    assertEq("fooBar12345\u1200".search("5\u1200"), 10);
    assertEq("fooBar12345\u1200".search("5\u1201"), -1);
}
testSearchFlat();
