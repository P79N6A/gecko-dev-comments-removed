





var x;
label
: {
    x = 1;
    break label;
    x = 2;
}
assertEq(x, 1);
reportCompare(0, 0, 'ok');
