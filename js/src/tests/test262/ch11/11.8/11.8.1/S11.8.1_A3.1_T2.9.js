










if (true < null !== false) {
  $ERROR('#1: true < null === false');
}


if (null < true !== true) {
  $ERROR('#2: null < true === true');
}


if (new Boolean(true) < null !== false) {
  $ERROR('#3: new Boolean(true) < null === false');
}


if (null < new Boolean(true) !== true) {
  $ERROR('#4: null < new Boolean(true) === true');
}

