










if (true === Number.NaN) {
  $ERROR('#1: true !== NaN');
}


if (-1 === Number.NaN) {
  $ERROR('#2: -1 !== NaN');
}


if (Number.NaN === Number.NaN) {
  $ERROR('#3: NaN !== NaN');
}


if (Number.POSITIVE_INFINITY === Number.NaN) {
  $ERROR('#4: +Infinity !== NaN');
}


if (Number.NEGATIVE_INFINITY === Number.NaN) {
  $ERROR('#5: -Infinity !== NaN');
}


if (Number.MAX_VALUE === Number.NaN) {
  $ERROR('#6: Number.MAX_VALUE !== NaN');
}


if (Number.MIN_VALUE === Number.NaN) {
  $ERROR('#7: Number.MIN_VALUE !== NaN');
}


if ("string" === Number.NaN) {
  $ERROR('#8: "string" !== NaN');
}


if (new Object() === Number.NaN) {
  $ERROR('#9: new Object() !== NaN');
}

