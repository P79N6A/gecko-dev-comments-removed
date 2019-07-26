










if (isNaN(Number.NEGATIVE_INFINITY % Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#1: -Infinity % Infinity === Not-a-Number. Actual: ' + (-Infinity % Infinity));
}


if (isNaN(Number.NEGATIVE_INFINITY % Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#2: -Infinity % -Infinity === Not-a-Number. Actual: ' + (-Infinity % -Infinity));
}


if (isNaN(Number.POSITIVE_INFINITY % Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#3: Infinity % Infinity === Not-a-Number. Actual: ' + (Infinity % Infinity));
}


if (isNaN(Number.POSITIVE_INFINITY % Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#4: Infinity % -Infinity === Not-a-Number. Actual: ' + (Infinity % -Infinity));
}


if (isNaN(Number.NEGATIVE_INFINITY % 1) !== true) {
  $ERROR('#5: Infinity % 1 === Not-a-Number. Actual: ' + (Infinity % 1));
}


if (isNaN(Number.NEGATIVE_INFINITY % -1) !== true) {
  $ERROR('#6: -Infinity % -1 === Not-a-Number. Actual: ' + (-Infinity % -1));
}


if (isNaN(Number.POSITIVE_INFINITY % 1) !== true) {
  $ERROR('#7: Infinity % 1 === Not-a-Number. Actual: ' + (Infinity % 1));
}


if (isNaN(Number.POSITIVE_INFINITY % -1) !== true) {
  $ERROR('#8: Infinity % -1 === Not-a-Number. Actual: ' + (Infinity % -1));
}


if (isNaN(Number.NEGATIVE_INFINITY % Number.MAX_VALUE) !== true) {
  $ERROR('#9: Infinity % Number.MAX_VALUE === Not-a-Number. Actual: ' + (Infinity % Number.MAX_VALUE));
}


if (isNaN(Number.NEGATIVE_INFINITY % -Number.MAX_VALUE) !== true) {
  $ERROR('#10: -Infinity % -Number.MAX_VALUE === Not-a-Number. Actual: ' + (-Infinity % -Number.MAX_VALUE));
}


if (isNaN(Number.POSITIVE_INFINITY % Number.MAX_VALUE) !== true) {
  $ERROR('#11: Infinity % Number.MAX_VALUE === Not-a-Number. Actual: ' + (Infinity % Number.MAX_VALUE));
}


if (isNaN(Number.POSITIVE_INFINITY % -Number.MAX_VALUE) !== true) {
  $ERROR('#12: Infinity % -Number.MAX_VALUE === Not-a-Number. Actual: ' + (Infinity % -Number.MAX_VALUE));
}

