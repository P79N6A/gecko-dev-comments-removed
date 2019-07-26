











try {
  x && (x = true);
  $ERROR('#1.1: x && (x = true) throw ReferenceError. Actual: ' + (x && (x = true)));
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x && (x = true) throw ReferenceError. Actual: ' + (e));
  }
}


if (((y = true) && y) !== true) {
  $ERROR('#2: ((y = true) && y) === true');
}


