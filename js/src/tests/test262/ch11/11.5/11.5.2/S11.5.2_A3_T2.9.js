










if (true / null !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: true / null === +Infinity. Actual: ' + (true / null));
}


if (null / true !== 0) {
  $ERROR('#2: null / true === 0. Actual: ' + (null / true));
}


if (new Boolean(true) / null !== Number.POSITIVE_INFINITY) {
  $ERROR('#3: new Boolean(true) / null === +Infinity. Actual: ' + (new Boolean(true) / null));
}


if (null / new Boolean(true) !== 0) {
  $ERROR('#4: null / new Boolean(true) === 0. Actual: ' + (null / new Boolean(true)));
}

