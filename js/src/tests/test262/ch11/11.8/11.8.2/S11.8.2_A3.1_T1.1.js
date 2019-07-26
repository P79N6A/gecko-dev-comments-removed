










if (true > true !== false) {
  $ERROR('#1: true > true === false');
}


if (new Boolean(true) > true !== false) {
  $ERROR('#2: new Boolean(true) > true === false');
}


if (true > new Boolean(true) !== false) {
  $ERROR('#3: true > new Boolean(true) === false');
}


if (new Boolean(true) > new Boolean(true) !== false) {
  $ERROR('#4: new Boolean(true) > new Boolean(true) === false');
}

