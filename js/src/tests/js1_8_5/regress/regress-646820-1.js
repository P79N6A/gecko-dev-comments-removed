


(function () {
    var [x, y] = [1, function () x];
    assertEq(y(), 1);
})();

reportCompare(0, 0, 'ok');
