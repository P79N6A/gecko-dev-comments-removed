










if ("1" >> "1" !== 0) {
  $ERROR('#1: "1" >> "1" === 0. Actual: ' + ("1" >> "1"));
}


if (new String("1") >> "1" !== 0) {
  $ERROR('#2: new String("1") >> "1" === 0. Actual: ' + (new String("1") >> "1"));
}


if ("1" >> new String("1") !== 0) {
  $ERROR('#3: "1" >> new String("1") === 0. Actual: ' + ("1" >> new String("1")));
}


if (new String("1") >> new String("1") !== 0) {
  $ERROR('#4: new String("1") >> new String("1") === 0. Actual: ' + (new String("1") >> new String("1")));
}


if ("x" >> "1" !== 0) {
  $ERROR('#5: "x" >> "1" === 0. Actual: ' + ("x" >> "1"));
}


if ("1" >> "x" !== 1) {
  $ERROR('#6: "1" >> "x" === 1. Actual: ' + ("1" >> "x"));
}

