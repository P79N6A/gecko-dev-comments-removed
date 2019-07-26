










if ((undefined != true) !== true) {
  $ERROR('#1: (undefined != true) === true');
}


if ((undefined != 0) !== true) {
  $ERROR('#2: (undefined != 0) === true');
}


if ((undefined != "undefined") !== true) {
  $ERROR('#3: (undefined != "undefined") === true');
}


if ((undefined != {}) !== true) {
  $ERROR('#4: (undefined != {}) === true');
}


if ((null != false) !== true) {
  $ERROR('#5: (null != false) === true');
}


if ((null != 0) !== true) {
  $ERROR('#6: (null != 0) === true');
}


if ((null != "null") !== true) {
  $ERROR('#7: (null != "null") === true');
}


if ((null != {}) !== true) {
  $ERROR('#8: (null != {}) === true');
}

