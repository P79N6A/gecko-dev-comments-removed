










if (isNaN(Number.NEGATIVE_INFINITY * 0) !== true) {
  $ERROR('#1: Infinity * 0 === Not-a-Number. Actual: ' + (Infinity * 0));
}


if (isNaN(-0 * Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#2: -0 * -Infinity === Not-a-Number. Actual: ' + (-0 * -Infinity));
}


if (isNaN(Number.POSITIVE_INFINITY * -0) !== true) {
  $ERROR('#3: Infinity * -0 === Not-a-Number. Actual: ' + (Infinity * -0));
}


if (isNaN(0 * Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#4: 0 * Infinity === Not-a-Number. Actual: ' + (0 * Infinity));
}


if (isNaN(Number.NEGATIVE_INFINITY * -0) !== true) {
  $ERROR('#5: Infinity * -0 === Not-a-Number. Actual: ' + (Infinity * -0));
}


if (isNaN(0 * Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#6: 0 * -Infinity === Not-a-Number. Actual: ' + (0 * -Infinity));
}


if (isNaN(Number.POSITIVE_INFINITY * 0) !== true) {
  $ERROR('#7: Infinity * 0 === Not-a-Number. Actual: ' + (Infinity * 0));
}


if (isNaN(-0 * Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#8: -0 * Infinity === Not-a-Number. Actual: ' + (-0 * Infinity));
}

