


(function () {
    var obj = {prop: 1};
    var [x, {prop: y}] = [function () y, obj];
    assertEq(y, 1);
    assertEq(x(), 1);
})();

reportCompare(0, 0, 'ok');
