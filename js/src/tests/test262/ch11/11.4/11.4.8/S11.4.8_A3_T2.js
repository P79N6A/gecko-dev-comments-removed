










if (~0.1 !== -1) {
  $ERROR('#1: ~0.1 === -1. Actual: ' + (~0.1));
}


if (~new Number(-0.1) !== -1) {
  $ERROR('#2: ~new Number(-0.1) === -1. Actual: ' + (~new Number(-0.1)));
}


if (~NaN !== -1) {
  $ERROR('#3: ~NaN === -1. Actual: ' + (~NaN));
}


if (~new Number(NaN) !== -1) {
  $ERROR('#4: ~new Number(NaN) === -1. Actual: ' + (~new Number(NaN)));
}


if (~1 !== -2) {
  $ERROR('#5: ~1 === -2. Actual: ' + (~1));
}


if (~new Number(-2) !== 1) {
  $ERROR('#6: ~new Number(-2) === 1. Actual: ' + (~new Number(-2)));
}


if (~Infinity !== -1) {
  $ERROR('#7: ~Infinity === -1. Actual: ' + (~Infinity));
}

