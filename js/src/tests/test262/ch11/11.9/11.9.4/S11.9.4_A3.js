











if (!(true === true)) {
  $ERROR('#1: true === true');
}


if (!(false === false)) {
  $ERROR('#2: false === false');
}


if (true === false) {
  $ERROR('#3: true !== false');
}


if (false === true) {
  $ERROR('#4: false !== true');
}

