










if ("1" >>> 1 !== 0) {
  $ERROR('#1: "1" >>> 1 === 0. Actual: ' + ("1" >>> 1));
}


if (1 >>> "1" !== 0) {
  $ERROR('#2: 1 >>> "1" === 0. Actual: ' + (1 >>> "1"));
}


if (new String("1") >>> 1 !== 0) {
  $ERROR('#3: new String("1") >>> 1 === 0. Actual: ' + (new String("1") >>> 1));
}


if (1 >>> new String("1") !== 0) {
  $ERROR('#4: 1 >>> new String("1") === 0. Actual: ' + (1 >>> new String("1")));
}


if ("1" >>> new Number(1) !== 0) {
  $ERROR('#5: "1" >>> new Number(1) === 0. Actual: ' + ("1" >>> new Number(1)));
}


if (new Number(1) >>> "1" !== 0) {
  $ERROR('#6: new Number(1) >>> "1" === 0. Actual: ' + (new Number(1) >>> "1"));
}


if (new String("1") >>> new Number(1) !== 0) {
  $ERROR('#7: new String("1") >>> new Number(1) === 0. Actual: ' + (new String("1") >>> new Number(1)));
}


if (new Number(1) >>> new String("1") !== 0) {
  $ERROR('#8: new Number(1) >>> new String("1") === 0. Actual: ' + (new Number(1) >>> new String("1")));
}


if ("x" >>> 1 !== 0) {
  $ERROR('#9: "x" >>> 1 === 0. Actual: ' + ("x" >>> 1));
}


if (1 >>> "x" !== 1) {
  $ERROR('#10: 1 >>> "x" === 1. Actual: ' + (1 >>> "x"));
}

