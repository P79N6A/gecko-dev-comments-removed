










if (isNaN(Number.NaN - 1) !== true ) {
  $ERROR('#1: NaN - 1 === Not-a-Number. Actual: ' + (NaN - 1));
}


if (isNaN(1 - Number.NaN) !== true ) {
  $ERROR('#2: 1 - NaN === Not-a-Number. Actual: ' + (1 - NaN));
}


if (isNaN(Number.NaN - Number.POSITIVE_INFINITY) !== true ) {
  $ERROR('#3: NaN - Infinity === Not-a-Number. Actual: ' + (NaN - Infinity));
}


if (isNaN(Number.POSITIVE_INFINITY - Number.NaN) !== true ) {
  $ERROR('#4: Infinity - NaN === Not-a-Number. Actual: ' + (Infinity - NaN));
}


if (isNaN(Number.NaN - Number.NEGATIVE_INFINITY) !== true ) {
  $ERROR('#5: NaN - Infinity === Not-a-Number. Actual: ' + (NaN - Infinity));
}


if (isNaN(Number.NEGATIVE_INFINITY - Number.NaN) !== true ) {
  $ERROR('#6: Infinity - NaN === Not-a-Number. Actual: ' + (Infinity - NaN));
}

