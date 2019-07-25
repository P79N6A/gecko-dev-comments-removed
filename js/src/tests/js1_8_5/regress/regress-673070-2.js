


var q = 1;
let ([q] = [eval("q")])
    assertEq(q, 1);

reportCompare(0, 0, 'ok');
