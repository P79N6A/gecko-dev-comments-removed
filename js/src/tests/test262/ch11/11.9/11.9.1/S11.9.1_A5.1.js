











if (("" == "") !== true) {
  $ERROR('#1: ("" == "") === true');
}


if ((" " == " ") !== true) {
  $ERROR('#2: " (" == " ") === true');
}


if ((" " == "") !== false) {
  $ERROR('#3: " (" == "") === false');
}


if (("string" == "string") !== true) {
  $ERROR('#4: ("string" == "string") === true');
}


if ((" string" == "string ") !== false) {
  $ERROR('#5: (" string" == "string ") === false');
}


if (("1.0" == "1") !== false) {
  $ERROR('#6: ("1.0" == "1") === false');
}


if (("0xff" == "255") !== false) {
  $ERROR('#7: ("0xff" == "255") === false');
}

