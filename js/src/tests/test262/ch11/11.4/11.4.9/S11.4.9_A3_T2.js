










if (!0.1 !== false) {
  $ERROR('#1: !0.1 === false');
}


if (!new Number(-0.1) !== false) {
  $ERROR('#2: !new Number(-0.1) === false');
}


if (!NaN !== true) {
  $ERROR('#3: !NaN === true');
}


if (!new Number(NaN) !== false) {
  $ERROR('#4: !new Number(NaN) === false');
}


if (!0 !== true) {
  $ERROR('#5: !0 === true');
}


if (!new Number(0) !== false) {
  $ERROR('#6: !new Number(0) === false');
}


if (!Infinity !== false) {
  $ERROR('#7: !Infinity === false');
}

