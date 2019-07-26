










if (true << "1" !== 2) {
  $ERROR('#1: true << "1" === 2. Actual: ' + (true << "1"));
}


if ("1" << true !== 2) {
  $ERROR('#2: "1" << true === 2. Actual: ' + ("1" << true));
}


if (new Boolean(true) << "1" !== 2) {
  $ERROR('#3: new Boolean(true) << "1" === 2. Actual: ' + (new Boolean(true) << "1"));
}


if ("1" << new Boolean(true) !== 2) {
  $ERROR('#4: "1" << new Boolean(true) === 2. Actual: ' + ("1" << new Boolean(true)));
}


if (true << new String("1") !== 2) {
  $ERROR('#5: true << new String("1") === 2. Actual: ' + (true << new String("1")));
}


if (new String("1") << true !== 2) {
  $ERROR('#6: new String("1") << true === 2. Actual: ' + (new String("1") << true));
}


if (new Boolean(true) << new String("1") !== 2) {
  $ERROR('#7: new Boolean(true) << new String("1") === 2. Actual: ' + (new Boolean(true) << new String("1")));
}


if (new String("1") << new Boolean(true) !== 2) {
  $ERROR('#8: new String("1") << new Boolean(true) === 2. Actual: ' + (new String("1") << new Boolean(true)));
}

