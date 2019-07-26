










if (-"1" !== -1) {
  $ERROR('#1: -"1" === -1. Actual: ' + (-"1"));
}


if (isNaN(-"x") !== true) {
  $ERROR('#2: -"x" === Not-a-Number. Actual: ' + (-"x"));
}


if (-new String("-1") !== 1) {
  $ERROR('#3: -new String("-1") === 1. Actual: ' + (-new String("-1")));
}

