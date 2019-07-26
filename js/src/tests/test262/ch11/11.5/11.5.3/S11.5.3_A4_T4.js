










if (isNaN(-0 % 0) !== true) {
  $ERROR('#1: -0 % 0 === Not-a-Number. Actual: ' + (-0 % 0));
}


if (isNaN(-0 % -0) !== true) {
  $ERROR('#2: -0 % -0 === Not-a-Number. Actual: ' + (-0 % -0));
}


if (isNaN(0 % 0) !== true) {
  $ERROR('#3: 0 % 0 === Not-a-Number. Actual: ' + (0 % 0));
}


if (isNaN(0 % -0) !== true) {
  $ERROR('#4: 0 % -0 === Not-a-Number. Actual: ' + (0 % -0));
}


if (isNaN(-1 % 0) !== true) {
  $ERROR('#5: 1 % 0 === Not-a-Number. Actual: ' + (1 % 0));
}


if (isNaN(-1 % -0) !== true) {
  $ERROR('#6: -1 % -0 === Not-a-Number. Actual: ' + (-1 % -0));
}


if (isNaN(1 % 0) !== true) {
  $ERROR('#7: 1 % 0 === Not-a-Number. Actual: ' + (1 % 0));
}


if (isNaN(1 % -0) !== true) {
  $ERROR('#8: 1 % -0 === Not-a-Number. Actual: ' + (1 % -0));
}


if (isNaN(Number.NEGATIVE_INFINITY % 0) !== true) {
  $ERROR('#9: Infinity % 0 === Not-a-Number. Actual: ' + (Infinity % 0));
}


if (isNaN(Number.NEGATIVE_INFINITY % -0) !== true) {
  $ERROR('#10: -Infinity % -0 === Not-a-Number. Actual: ' + (-Infinity % -0));
}


if (isNaN(Number.POSITIVE_INFINITY % 0) !== true) {
  $ERROR('#11: Infinity % 0 === Not-a-Number. Actual: ' + (Infinity % 0));
}


if (isNaN(Number.POSITIVE_INFINITY % -0) !== true) {
  $ERROR('#12: Infinity % -0 === Not-a-Number. Actual: ' + (Infinity % -0));
}


if (isNaN(Number.MIN_VALUE % 0) !== true) {
  $ERROR('#13: Number.MIN_VALUE % 0 === Not-a-Number. Actual: ' + (Number.MIN_VALUE % 0));
}


if (isNaN(Number.MIN_VALUE % -0) !== true) {
  $ERROR('#14: -Number.MIN_VALUE % -0 === Not-a-Number. Actual: ' + (-Number.MIN_VALUE % -0));
}


if (isNaN(Number.MAX_VALUE % 0) !== true) {
  $ERROR('#15: Number.MAX_VALUE % 0 === Not-a-Number. Actual: ' + (Number.MAX_VALUE % 0));
}


if (isNaN(Number.MAX_VALUE % -0) !== true) {
  $ERROR('#16: Number.MAX_VALUE % -0 === Not-a-Number. Actual: ' + (Number.MAX_VALUE % -0));
}

