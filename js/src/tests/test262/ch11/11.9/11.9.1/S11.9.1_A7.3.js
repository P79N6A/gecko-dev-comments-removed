











if ((true == new Boolean(true)) !== true) {
  $ERROR('#1: (true == new Boolean(true)) === true');
}


if ((true == new Number(1)) !== true) {
  $ERROR('#2: (true == new Number(1)) === true');
}


if ((true == new String("+1")) !== true) {
  $ERROR('#3: (true == new String("+1")) === true');
}

