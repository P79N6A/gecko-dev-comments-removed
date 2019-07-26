










if (isNaN(1 * undefined) !== true) {
  $ERROR('#1: 1 * undefined === Not-a-Number. Actual: ' + (1 * undefined));
}


if (isNaN(undefined * 1) !== true) {
  $ERROR('#2: undefined * 1 === Not-a-Number. Actual: ' + (undefined * 1));
}


if (isNaN(new Number(1) * undefined) !== true) {
  $ERROR('#3: new Number(1) * undefined === Not-a-Number. Actual: ' + (new Number(1) * undefined));
}


if (isNaN(undefined * new Number(1)) !== true) {
  $ERROR('#4: undefined * new Number(1) === Not-a-Number. Actual: ' + (undefined * new Number(1)));
}

