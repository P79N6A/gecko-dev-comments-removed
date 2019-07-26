










if (true + 1 !== 2) {
  $ERROR('#1: true + 1 === 2. Actual: ' + (true + 1));
}


if (1 + true !== 2) {
  $ERROR('#2: 1 + true === 2. Actual: ' + (1 + true));
}


if (new Boolean(true) + 1 !== 2) {
  $ERROR('#3: new Boolean(true) + 1 === 2. Actual: ' + (new Boolean(true) + 1));
}


if (1 + new Boolean(true) !== 2) {
  $ERROR('#4: 1 + new Boolean(true) === 2. Actual: ' + (1 + new Boolean(true)));
}


if (true + new Number(1) !== 2) {
  $ERROR('#5: true + new Number(1) === 2. Actual: ' + (true + new Number(1)));
}


if (new Number(1) + true !== 2) {
  $ERROR('#6: new Number(1) + true === 2. Actual: ' + (new Number(1) + true));
}


if (new Boolean(true) + new Number(1) !== 2) {
  $ERROR('#7: new Boolean(true) + new Number(1) === 2. Actual: ' + (new Boolean(true) + new Number(1)));
}


if (new Number(1) + new Boolean(true) !== 2) {
  $ERROR('#8: new Number(1) + new Boolean(true) === 2. Actual: ' + (new Number(1) + new Boolean(true)));
}

