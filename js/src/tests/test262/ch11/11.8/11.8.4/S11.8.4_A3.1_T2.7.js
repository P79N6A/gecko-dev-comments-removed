










if ("1" >= null !== true) {
  $ERROR('#1: "1" >= null === true');
}


if (null >= "1" !== false) {
  $ERROR('#2: null >= "1" === false');
}


if (new String("1") >= null !== true) {
  $ERROR('#3: new String("1") >= null === true');
}


if (null >= new String("1") !== false) {
  $ERROR('#4: null >= new String("1") === false');
}

