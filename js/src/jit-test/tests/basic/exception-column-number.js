try {
    Array.indexOf();
} catch (e) {
    assertEq(e.columnNumber, 5);
    
    
    
    var lastColon = e.stack.lastIndexOf(':');
    var afterPath = e.stack.lastIndexOf(':', lastColon - 1);
    assertEq(e.stack.substring(afterPath), ":2:5\n");
}
