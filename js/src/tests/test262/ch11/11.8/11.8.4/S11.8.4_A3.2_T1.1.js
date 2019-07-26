










if ("1" >= "1" !== true) {
  $ERROR('#1: "1" >= "1" === true');
}


if (new String("1") >= "1" !== true) {
  $ERROR('#2: new String("1") >= "1" === true');
}


if ("1" >= new String("1") !== true) {
  $ERROR('#3: "1" >= new String("1") === true');
}


if (new String("1") >= new String("1") !== true) {
  $ERROR('#4: new String("1") >= new String("1") === true');
}


if ("x" >= "1" !== true) {
  $ERROR('#5: "x" >= "1" === true');
}


if ("1" >= "x" !== false) {
  $ERROR('#6: "1" >= "x" === false');
}

