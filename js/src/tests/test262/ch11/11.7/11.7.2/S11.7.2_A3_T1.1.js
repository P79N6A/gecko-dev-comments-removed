










if (true >> true !== 0) {
  $ERROR('#1: true >> true === 0. Actual: ' + (true >> true));
}


if (new Boolean(true) >> true !== 0) {
  $ERROR('#2: new Boolean(true) >> true === 0. Actual: ' + (new Boolean(true) >> true));
}


if (true >> new Boolean(true) !== 0) {
  $ERROR('#3: true >> new Boolean(true) === 0. Actual: ' + (true >> new Boolean(true)));
}


if (new Boolean(true) >> new Boolean(true) !== 0) {
  $ERROR('#4: new Boolean(true) >> new Boolean(true) === 0. Actual: ' + (new Boolean(true) >> new Boolean(true)));
}

