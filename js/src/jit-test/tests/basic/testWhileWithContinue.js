


var i = 0;
while (i < HOTLOOP+4) {
    ++i;
    continue;
}
assertEq(i, HOTLOOP+4);

