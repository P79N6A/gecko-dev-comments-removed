










if ((-2147483647 << 0) !== -2147483647) {
  $ERROR('#1: (-2147483647 << 0) === -2147483647. Actual: ' + ((-2147483647 << 0)));
}


if ((-2147483648 << 0) !== -2147483648) {
  $ERROR('#2: (-2147483648 << 0) === -2147483648. Actual: ' + ((-2147483648 << 0)));
}


if ((-2147483649 << 0) !== 2147483647) {
  $ERROR('#3: (-2147483649 << 0) === 2147483647. Actual: ' + ((-2147483649 << 0)));
}


if ((-4294967296 << 0) !== 0) {
  $ERROR('#4: (-4294967296 << 0) === 0. Actual: ' + ((-4294967296 << 0)));
}


if ((2147483646 << 0) !== 2147483646) {
  $ERROR('#5: (2147483646 << 0) === 2147483646. Actual: ' + ((2147483646 << 0)));
}


if ((2147483647 << 0) !== 2147483647) {
  $ERROR('#6: (2147483647 << 0) === 2147483647. Actual: ' + ((2147483647 << 0)));
}


if ((2147483648 << 0) !== -2147483648) {
  $ERROR('#7: (2147483648 << 0) === -2147483648. Actual: ' + ((2147483648 << 0)));
}


if ((4294967296 << 0) !== 0) {
  $ERROR('#8: (4294967296 << 0) === 0. Actual: ' + ((4294967296 << 0)));
}

