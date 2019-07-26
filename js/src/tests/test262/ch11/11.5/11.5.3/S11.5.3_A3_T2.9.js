










if (isNaN(true % null) !== true) {
  $ERROR('#1: true % null === Not-a-Number. Actual: ' + (true % null));
}


if (null % true !== 0) {
  $ERROR('#2: null % true === 0. Actual: ' + (null % true));
}


if (isNaN(new Boolean(true) % null) !== true) {
  $ERROR('#3: new Boolean(true) % null === Not-a-Number. Actual: ' + (new Boolean(true) % null));
}


if (null % new Boolean(true) !== 0) {
  $ERROR('#4: null % new Boolean(true) === 0. Actual: ' + (null % new Boolean(true)));
}

