










if (!"1" !== false) {
  $ERROR('#1: !"1" === false');
}


if (!new String("0") !== false) {
  $ERROR('#2: !new String("0") === false');
}


if (!"x" !== false) {
  $ERROR('#3: !"x" === false');
}


if (!"" !== true) {
  $ERROR('#4: !"" === true');
}


if (!new String("") !== false) {
  $ERROR('#5: !new String("") === false');
}

