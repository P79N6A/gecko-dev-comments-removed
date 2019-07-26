










if (!(Number.NaN !== true)) {
  $ERROR('#1: NaN !== true');
}


if (!(Number.NaN !== 1)) {
  $ERROR('#2: NaN !== 1');
}


if (!(Number.NaN !== Number.NaN)) {
  $ERROR('#3: NaN !== NaN');
}


if (!(Number.NaN !== Number.POSITIVE_INFINITY)) {
  $ERROR('#4: NaN !== +Infinity');
}


if (!(Number.NaN !== Number.NEGATIVE_INFINITY)) {
  $ERROR('#5: NaN !== -Infinity');
}


if (!(Number.NaN !== Number.MAX_VALUE)) {
  $ERROR('#6: NaN !== Number.MAX_VALUE');
}


if (!(Number.NaN !== Number.MIN_VALUE)) {
  $ERROR('#7: NaN !== Number.MIN_VALUE');
}


if (!(Number.NaN !== "string")) {
  $ERROR('#8: NaN !== "string"');
}


if (!(Number.NaN !== new Object())) {
  $ERROR('#9: NaN !== new Object()');
}


