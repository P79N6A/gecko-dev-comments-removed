










if (isNaN(null * undefined) !== true) {
  $ERROR('#1: null * undefined === Not-a-Number. Actual: ' + (null * undefined));
}


if (isNaN(undefined * null) !== true) {
  $ERROR('#2: undefined * null === Not-a-Number. Actual: ' + (undefined * null));
}


if (isNaN(undefined * undefined) !== true) {
  $ERROR('#3: undefined * undefined === Not-a-Number. Actual: ' + (undefined * undefined));
}


if (null * null !== 0) {
  $ERROR('#4: null * null === 0. Actual: ' + (null * null));
}

