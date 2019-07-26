










if (undefined !== undefined) {
  $ERROR('#1: undefined === undefined');
}


if (void 0 !== undefined) {
  $ERROR('#2: void 0 === undefined');
}


if (undefined !== eval("var x")) {
  $ERROR('#3: undefined === eval("var x")');
}

