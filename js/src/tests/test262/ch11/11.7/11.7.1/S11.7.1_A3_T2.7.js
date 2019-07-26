










if ("1" << null !== 1) {
  $ERROR('#1: "1" << null === 1. Actual: ' + ("1" << null));
}


if (null << "1" !== 0) {
  $ERROR('#2: null << "1" === 0. Actual: ' + (null << "1"));
}


if (new String("1") << null !== 1) {
  $ERROR('#3: new String("1") << null === 1. Actual: ' + (new String("1") << null));
}


if (null << new String("1") !== 0) {
  $ERROR('#4: null << new String("1") === 0. Actual: ' + (null << new String("1")));
}

