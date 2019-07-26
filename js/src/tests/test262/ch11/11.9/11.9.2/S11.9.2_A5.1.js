











if (("" != "") !== false) {
  $ERROR('#1: ("" != "") === false');
}


if ((" " != " ") !== false) {
  $ERROR('#2: " (" != " ") === false');
}


if ((" " != "") !== true) {
  $ERROR('#3: " (" != "") === true');
}


if (("string" != "string") !== false) {
  $ERROR('#4: ("string" != "string") === false');
}


if ((" string" != "string ") !== true) {
  $ERROR('#5: (" string" != "string ") === true');
}


if (("1.0" != "1") !== true) {
  $ERROR('#6: ("1.0" != "1") === true');
}


if (("0xff" != "255") !== true) {
  $ERROR('#7: ("0xff" != "255") === true');
}

