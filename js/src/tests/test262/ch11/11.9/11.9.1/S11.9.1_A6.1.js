










if ((undefined == undefined) !== true) {
  $ERROR('#1: (undefined == undefined) === true');
}


if ((void 0 == undefined) !== true) {
  $ERROR('#2: (void 0 == undefined) === true');
}


if ((undefined == eval("var x")) !== true) {
  $ERROR('#3: (undefined == eval("var x")) === true');
}


if ((undefined == null) !== true) {
  $ERROR('#4: (undefined == null) === true');
}


if ((null == void 0) !== true) {
  $ERROR('#5: (null == void 0) === true');
}


if ((null == null) !== true) {
  $ERROR('#6: (null == null) === true');
}

