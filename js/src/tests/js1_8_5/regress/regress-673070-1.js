


var q = [2];
let ([q] = eval("q"))
    assertEq(q, 2);

reportCompare(0, 0, 'ok');
