











if ((1 != "1") !== false) {
  $ERROR('#1: (1 != "1") === false');
}


if ((1.100 != "+1.10") !== false) {
  $ERROR('#2: (1.100 != "+1.10") === false');
}


if ((1 != "true") !== true) {
  $ERROR('#3: (1 != "true") === true');
}


if ((255 != "0xff") !== false) {
  $ERROR('#4: (255 != "0xff") === false');
}


if ((0 != "") !== false) {
  $ERROR('#5: (0 != "") === false');
}

