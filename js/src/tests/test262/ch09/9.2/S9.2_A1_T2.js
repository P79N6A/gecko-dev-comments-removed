










if (!(undefined) !== true) {
  $ERROR('#1: !(undefined) === true. Actual: ' + (!(undefined)));
}


if (!(void 0) !== true) {
  $ERROR('#2: !(undefined) === true. Actual: ' + (!(undefined)));
}


if (!(eval("var x")) !== true) {
  $ERROR('#3: !(eval("var x")) === true. Actual: ' + (!(eval("var x"))));
}

