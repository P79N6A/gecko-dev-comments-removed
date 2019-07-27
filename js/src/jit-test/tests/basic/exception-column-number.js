try {
    Array.indexOf();
} catch (e) {
    assertEq(e.columnNumber, 5);
    
    
    assertEq(e.stack.replace(/[^:]*/, ""), ":2:5\n");
}
