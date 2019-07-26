










if ("1" / 1 !== 1) {
  $ERROR('#1: "1" / 1 === 1. Actual: ' + ("1" / 1));
}


if (1 / "1" !== 1) {
  $ERROR('#2: 1 / "1" === 1. Actual: ' + (1 / "1"));
}


if (new String("1") / 1 !== 1) {
  $ERROR('#3: new String("1") / 1 === 1. Actual: ' + (new String("1") / 1));
}


if (1 / new String("1") !== 1) {
  $ERROR('#4: 1 / new String("1") === 1. Actual: ' + (1 / new String("1")));
}


if ("1" / new Number(1) !== 1) {
  $ERROR('#5: "1" / new Number(1) === 1. Actual: ' + ("1" / new Number(1)));
}


if (new Number(1) / "1" !== 1) {
  $ERROR('#6: new Number(1) / "1" === 1. Actual: ' + (new Number(1) / "1"));
}


if (new String("1") / new Number(1) !== 1) {
  $ERROR('#7: new String("1") / new Number(1) === 1. Actual: ' + (new String("1") / new Number(1)));
}


if (new Number(1) / new String("1") !== 1) {
  $ERROR('#8: new Number(1) / new String("1") === 1. Actual: ' + (new Number(1) / new String("1")));
}


if (isNaN("x" / 1) !== true) {
  $ERROR('#9: "x" / 1 === Not-a-Number. Actual: ' + ("x" / 1));
}


if (isNaN(1 / "x") !== true) {
  $ERROR('#10: 1 / "x" === Not-a-Number. Actual: ' + (1 / "x"));
}

