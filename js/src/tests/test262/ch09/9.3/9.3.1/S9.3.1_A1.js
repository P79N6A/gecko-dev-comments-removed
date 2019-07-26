










if (Number("") !== 0) {
  $ERROR('#1.1: Number("") === 0. Actual: ' + (Number("")));
} else {
  if (1/Number("") !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: Number("") == +0. Actual: -0');
  }
}

