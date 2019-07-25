
assertRaises(StopIteration, function() {
    Iterator.prototype.next();
    Iterator.prototype.next();
});


assertRaises(StopIteration, function() {
    Iterator.prototype.next();
});


assertRaises(StopIteration, function() {
    (new Iterator({})).__proto__.next();
});


function assertRaises(exc, callback) {
    var caught = false;
    try {
        callback();
    } catch (e) {
        assertEq(e instanceof InternalError, true);
        caught = true;
    }
    assertEq(caught, true);
}
