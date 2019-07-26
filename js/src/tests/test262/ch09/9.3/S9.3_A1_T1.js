










if (isNaN(Number(undefined)) !== true) {
  $ERROR('#1: Number(undefined) === Not-a-Number. Actual: ' + (Number(undefined)));
}


if (isNaN(Number(void 0)) !== true) {
  $ERROR('#2: Number(void 0) === Not-a-Number. Actual: ' + (Number(void 0)));
}


if (isNaN(Number(eval("var x"))) !== true) {
  $ERROR('#3: Number(eval("var x")) === Not-a-Number. Actual: ' + (Number(eval("var x"))));
}

