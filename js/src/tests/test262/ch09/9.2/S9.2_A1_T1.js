










if (Boolean(undefined) !== false) {
  $ERROR('#1: Boolean(undefined) === false. Actual: ' + (Boolean(undefined)));
}


if (Boolean(void 0) !== false) {
  $ERROR('#2: Boolean(undefined) === false. Actual: ' + (Boolean(undefined)));
}


if (Boolean(eval("var x")) !== false) {
  $ERROR('#3: Boolean(eval("var x")) === false. Actual: ' + (Boolean(eval("var x"))));
}


if (Boolean() !== false) {
  $ERROR('#4: Boolean() === false. Actual: ' + (Boolean()));
}

