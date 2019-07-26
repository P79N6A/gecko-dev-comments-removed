











if (Number.POSITIVE_INFINITY !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: +Infinity === +Infinity');
}


if (Number.NEGATIVE_INFINITY !== Number.NEGATIVE_INFINITY) {
  $ERROR('#2: -Infinity === -Infinity');
}


if (13 !== 13) {
  $ERROR('#3: 13 === 13');
}


if (-13 !== -13) {
  $ERROR('#4: -13 === -13');
}


if (1.3 !== 1.3) {
  $ERROR('#5: 1.3 === 1.3');
}


if (-1.3 !== -1.3) {
  $ERROR('#6: -1.3 === -1.3');
}


if (Number.POSITIVE_INFINITY !== -Number.NEGATIVE_INFINITY) {
  $ERROR('#7: +Infinity === -(-Infinity)');
}


if (!(1 !== 0.999999999999)) {
  $ERROR('#8: 1 !== 0.999999999999');
}


if (1.0 !== 1) {
  $ERROR('#9: 1.0 === 1');
}

