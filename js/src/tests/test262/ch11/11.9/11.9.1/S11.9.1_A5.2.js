











if ((1 == "1") !== true) {
  $ERROR('#1: (1 == "1") === true');
}


if ((1.100 == "+1.10") !== true) {
  $ERROR('#2: (1.100 == "+1.10") === true');
}


if ((1 == "true") !== false) {
  $ERROR('#3: (1 == "true") === false');
}


if ((255 == "0xff") !== true) {
  $ERROR('#4: (255 == "0xff") === true');
}


if ((0 == "") !== true) {
  $ERROR('#5: (0 == "") === true');
}

