










if (true > undefined !== false) {
  $ERROR('#1: true > undefined === false');
}


if (undefined > true !== false) {
  $ERROR('#2: undefined > true === false');
}


if (new Boolean(true) > undefined !== false) {
  $ERROR('#3: new Boolean(true) > undefined === false');
}


if (undefined > new Boolean(true) !== false) {
  $ERROR('#4: undefined > new Boolean(true) === false');
}

