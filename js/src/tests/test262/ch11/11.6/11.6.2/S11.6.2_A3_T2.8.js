










if (isNaN(true - undefined) !== true) {
  $ERROR('#1: true - undefined === Not-a-Number. Actual: ' + (true - undefined));
}


if (isNaN(undefined - true) !== true) {
  $ERROR('#2: undefined - true === Not-a-Number. Actual: ' + (undefined - true));
}


if (isNaN(new Boolean(true) - undefined) !== true) {
  $ERROR('#3: new Boolean(true) - undefined === Not-a-Number. Actual: ' + (new Boolean(true) - undefined));
}


if (isNaN(undefined - new Boolean(true)) !== true) {
  $ERROR('#4: undefined - new Boolean(true) === Not-a-Number. Actual: ' + (undefined - new Boolean(true)));
}

