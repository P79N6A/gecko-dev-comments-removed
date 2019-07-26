










if ((1 >= 1) !== true) {
  $ERROR('#1: (1 >= 1) === true');
}


if ((1.1 >= 1.1) !== true) {
  $ERROR('#2: (1.1 >= 1.1) === true');
}


if ((-1.1 >= -1.1) !== true) {
  $ERROR('#3: (-1.1 >= -1.1) === true');
}


if ((Number.NEGATIVE_INFINITY >= Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#4: (-Infinity >= -Infinity) === true');
}


if ((Number.POSITIVE_INFINITY >= Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#5: (+Infinity >= +Infinity) === true');
}


if ((Number.MAX_VALUE >= Number.MAX_VALUE) !== true) {
  $ERROR('#6: (Number.MAX_VALUE >= Number.MAX_VALUE) === true');
}


if ((Number.MIN_VALUE >= Number.MIN_VALUE) !== true) {
  $ERROR('#7: (Number.MIN_VALUE >= Number.MIN_VALUE) === true');
}



