










if ((undefined == true) !== false) {
  $ERROR('#1: (undefined == true) === false');
}


if ((undefined == 0) !== false) {
  $ERROR('#2: (undefined == 0) === false');
}


if ((undefined == "undefined") !== false) {
  $ERROR('#3: (undefined == "undefined") === false');
}


if ((undefined == {}) !== false) {
  $ERROR('#4: (undefined == {}) === false');
}


if ((null == false) !== false) {
  $ERROR('#5: (null == false) === false');
}


if ((null == 0) !== false) {
  $ERROR('#6: (null == 0) === false');
}


if ((null == "null") !== false) {
  $ERROR('#7: (null == "null") === false');
}


if ((null == {}) !== false) {
  $ERROR('#8: (null == {}) === false');
}

