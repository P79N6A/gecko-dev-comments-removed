










if (~"1" !== -2) {
  $ERROR('#1: ~"1" === -2. Actual: ' + (~"1"));
}


if (~new String("0") !== -1) {
  $ERROR('#2: ~new String("0") === -1. Actual: ' + (~new String("0")));
}


if (~"x" !== -1) {
  $ERROR('#3: ~"x" === -1. Actual: ' + (~"x"));
}


if (~"" !== -1) {
  $ERROR('#4: ~"" === -1. Actual: ' + (~""));
}


if (~new String("-2") !== 1) {
  $ERROR('#5: ~new String("-2") === 1. Actual: ' + (~new String("-2")));
}

