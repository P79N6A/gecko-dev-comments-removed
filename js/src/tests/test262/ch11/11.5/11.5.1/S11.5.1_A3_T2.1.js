










if (true * 1 !== 1) {
  $ERROR('#1: true * 1 === 1. Actual: ' + (true * 1));
}


if (1 * true !== 1) {
  $ERROR('#2: 1 * true === 1. Actual: ' + (1 * true));
}


if (new Boolean(true) * 1 !== 1) {
  $ERROR('#3: new Boolean(true) * 1 === 1. Actual: ' + (new Boolean(true) * 1));
}


if (1 * new Boolean(true) !== 1) {
  $ERROR('#4: 1 * new Boolean(true) === 1. Actual: ' + (1 * new Boolean(true)));
}


if (true * new Number(1) !== 1) {
  $ERROR('#5: true * new Number(1) === 1. Actual: ' + (true * new Number(1)));
}


if (new Number(1) * true !== 1) {
  $ERROR('#6: new Number(1) * true === 1. Actual: ' + (new Number(1) * true));
}


if (new Boolean(true) * new Number(1) !== 1) {
  $ERROR('#7: new Boolean(true) * new Number(1) === 1. Actual: ' + (new Boolean(true) * new Number(1)));
}


if (new Number(1) * new Boolean(true) !== 1) {
  $ERROR('#8: new Number(1) * new Boolean(true) === 1. Actual: ' + (new Number(1) * new Boolean(true)));
}

