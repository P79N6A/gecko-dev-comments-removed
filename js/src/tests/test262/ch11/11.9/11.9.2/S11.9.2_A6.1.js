










if ((undefined != undefined) !== false) {
  $ERROR('#1: (undefined != undefined) === false');
}


if ((void 0 != undefined) !== false) {
  $ERROR('#2: (void 0 != undefined) === false');
}


if ((undefined != eval("var x")) !== false) {
  $ERROR('#3: (undefined != eval("var x")) === false');
}


if ((undefined != null) !== false) {
  $ERROR('#4: (undefined != null) === false');
}


if ((null != void 0) !== false) {
  $ERROR('#5: (null != void 0) === false');
}


if ((null != null) !== false) {
  $ERROR('#6: (null != null) === false');
}

