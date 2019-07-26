










if (isNaN(+(undefined)) !== true) {
  $ERROR('#1: +(undefined) === Not-a-Number. Actual: ' + (+(undefined)));
}


if (isNaN(+(void 0)) !== true) {
  $ERROR('#2: +(void 0) === Not-a-Number. Actual: ' + (+(void 0)));
}


if (isNaN(+(eval("var x"))) !== true) {
  $ERROR('#3: +(eval("var x")) === Not-a-Number. Actual: ' + (+(eval("var x"))));
}

