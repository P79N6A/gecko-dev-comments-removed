










if ("1" / null !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: "1" / null === +Infinity. Actual: ' + ("1" / null));
}


if (null / "1" !== 0) {
  $ERROR('#2: null / "1" === 0. Actual: ' + (null / "1"));
}


if (new String("1") / null !== Number.POSITIVE_INFINITY) {
  $ERROR('#3: new String("1") / null === +Infinity. Actual: ' + (new String("1") / null));
}


if (null / new String("1") !== 0) {
  $ERROR('#4: null / new String("1") === 0. Actual: ' + (null / new String("1")));
}

