










if ("1" > 1 !== false) {
  $ERROR('#1: "1" > 1 === false');
}


if (1 > "1" !== false) {
  $ERROR('#2: 1 > "1" === false');
}


if (new String("1") > 1 !== false) {
  $ERROR('#3: new String("1") > 1 === false');
}


if (1 > new String("1") !== false) {
  $ERROR('#4: 1 > new String("1") === false');
}


if ("1" > new Number(1) !== false) {
  $ERROR('#5: "1" > new Number(1) === false');
}


if (new Number(1) > "1" !== false) {
  $ERROR('#6: new Number(1) > "1" === false');
}


if (new String("1") > new Number(1) !== false) {
  $ERROR('#7: new String("1") > new Number(1) === false');
}


if (new Number(1) > new String("1") !== false) {
  $ERROR('#8: new Number(1) > new String("1") === false');
}


if ("x" > 1 !== false) {
  $ERROR('#9: "x" > 1 === false');
}


if (1 > "x" !== false) {
  $ERROR('#10: 1 > "x" === false');
}

