










if ("1" >>> undefined !== 1) {
  $ERROR('#1: "1" >>> undefined === 1. Actual: ' + ("1" >>> undefined));
}


if (undefined >>> "1" !== 0) {
  $ERROR('#2: undefined >>> "1" === 0. Actual: ' + (undefined >>> "1"));
}


if (new String("1") >>> undefined !== 1) {
  $ERROR('#3: new String("1") >>> undefined === 1. Actual: ' + (new String("1") >>> undefined));
}


if (undefined >>> new String("1") !== 0) {
  $ERROR('#4: undefined >>> new String("1") === 0. Actual: ' + (undefined >>> new String("1")));
}

