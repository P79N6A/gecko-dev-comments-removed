










if (isNaN("1" % null) !== true) {
  $ERROR('#1: "1" % null === Not-a-Number. Actual: ' + ("1" % null));
}


if (null % "1" !== 0) {
  $ERROR('#2: null % "1" === 0. Actual: ' + (null % "1"));
}


if (isNaN(new String("1") % null) !== true) {
  $ERROR('#3: new String("1") % null === Not-a-Number. Actual: ' + (new String("1") % null));
}


if (null % new String("1") !== 0) {
  $ERROR('#4: null % new String("1") === 0. Actual: ' + (null % new String("1")));
}

