











if (("-1" == -1) !== true) {
  $ERROR('#1: ("-1" == -1) === true');
}


if (("-1.100" == -1.10) !== true) {
  $ERROR('#2: ("-1.100" == -1.10) === true');
}


if (("false" == 0) !== false) {
  $ERROR('#3: ("false" == 0) === false');
}


if (("5e-324" == 5e-324) !== true) {
  $ERROR('#4: ("5e-324" == 5e-324) === true');
}


