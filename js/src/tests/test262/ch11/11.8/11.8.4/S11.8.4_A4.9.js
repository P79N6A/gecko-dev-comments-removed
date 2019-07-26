










if ((1 >= 1.1) !== false) {
  $ERROR('#1: (1 >= 1.1) === false');
}


if ((1.1 >= 1) !== true) {
  $ERROR('#2: (1.1 >= 1) === true');
}


if ((-1 >= -1.1) !== true) {
  $ERROR('#3: (-1 >= -1.1) === true');
}


if ((-1.1 >= -1) !== false) {
  $ERROR('#4: (-1.1 >= -1) === false');
}


if ((0.1 >= 0) !== true) {
  $ERROR('#5: (0.1 >= 0) === true');
}


if ((0 >= -0.1) !== true) {
  $ERROR('#6: (0 >= -0.1) === true');
}


if ((Number.MAX_VALUE >= Number.MAX_VALUE/2) !== true) {
  $ERROR('#7: (Number.MAX_VALUE >= Number.MAX_VALUE/2) === true');
}


if ((Number.MIN_VALUE*2 >= Number.MIN_VALUE) !== true) {
  $ERROR('#8: (Number.MIN_VALUE*2 >= Number.MIN_VALUE) === true');
}



