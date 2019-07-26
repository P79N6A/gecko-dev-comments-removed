











if (("-1" != -1) !== false) {
  $ERROR('#1: ("-1" != -1) === false');
}


if (("-1.100" != -1.10) !== false) {
  $ERROR('#2: ("-1.100" != -1.10) === false');
}


if (("false" != 0) !== true) {
  $ERROR('#3: ("false" != 0) === true');
}


if (("5e-324" != 5e-324) !== false) {
  $ERROR('#4: ("5e-324" != 5e-324) === false');
}


