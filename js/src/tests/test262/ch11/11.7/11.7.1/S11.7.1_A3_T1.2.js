










if (1 << 1 !== 2) {
  $ERROR('#1: 1 << 1 === 2. Actual: ' + (1 << 1));
}


if (new Number(1) << 1 !== 2) {
  $ERROR('#2: new Number(1) << 1 === 2. Actual: ' + (new Number(1) << 1));
}


if (1 << new Number(1) !== 2) {
  $ERROR('#3: 1 << new Number(1) === 2. Actual: ' + (1 << new Number(1)));
}


if (new Number(1) << new Number(1) !== 2) {
  $ERROR('#4: new Number(1) << new Number(1) === 2. Actual: ' + (new Number(1) << new Number(1)));
}


