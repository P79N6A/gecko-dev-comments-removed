

assertEq(Map(x for (x of [])).size(), 0);

function none() {
    if (0) yield 0;
}
assertEq(Map(none()).size(), 0);
