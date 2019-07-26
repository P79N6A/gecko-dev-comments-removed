










if (!false !== true) {
  $ERROR('#1: !false === true');
}


if (!new Boolean(true) !== false) {
  $ERROR('#2: !new Boolean(true) === false');
}


if (!new Boolean(false) !== false) {
  $ERROR('#3: !new Boolean(false) === false');
}

