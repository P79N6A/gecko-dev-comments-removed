










if (null <= undefined !== false) {
  $ERROR('#1: null <= undefined === false');
}


if (undefined <= null !== false) {
  $ERROR('#2: undefined <= null === false');
}


if (undefined <= undefined !== false) {
  $ERROR('#3: undefined <= undefined === false');
}


if (null <= null !== true) {
  $ERROR('#4: null <= null === true');
}

