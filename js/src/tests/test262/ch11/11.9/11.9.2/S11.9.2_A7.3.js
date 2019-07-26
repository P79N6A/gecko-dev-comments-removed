











if ((true != new Boolean(true)) !== false) {
  $ERROR('#1: (true != new Boolean(true)) === false');
}


if ((true != new Number(1)) !== false) {
  $ERROR('#2: (true != new Number(1)) === false');
}


if ((true != new String("+1")) !== false) {
  $ERROR('#3: (true != new String("+1")) === false');
}

