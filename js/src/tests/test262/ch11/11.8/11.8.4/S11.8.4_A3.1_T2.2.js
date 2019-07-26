










if ("1" >= 1 !== true) {
  $ERROR('#1: "1" >= 1 === true');
}


if (1 >= "1" !== true) {
  $ERROR('#2: 1 >= "1" === true');
}


if (new String("1") >= 1 !== true) {
  $ERROR('#3: new String("1") >= 1 === true');
}


if (1 >= new String("1") !== true) {
  $ERROR('#4: 1 >= new String("1") === true');
}


if ("1" >= new Number(1) !== true) {
  $ERROR('#5: "1" >= new Number(1) === true');
}


if (new Number(1) >= "1" !== true) {
  $ERROR('#6: new Number(1) >= "1" === true');
}


if (new String("1") >= new Number(1) !== true) {
  $ERROR('#7: new String("1") >= new Number(1) === true');
}


if (new Number(1) >= new String("1") !== true) {
  $ERROR('#8: new Number(1) >= new String("1") === true');
}


if ("x" >= 1 !== false) {
  $ERROR('#9: "x" >= 1 === false');
}


if (1 >= "x" !== false) {
  $ERROR('#10: 1 >= "x" === false');
}

