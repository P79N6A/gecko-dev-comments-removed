










if ("1" <= null !== false) {
  $ERROR('#1: "1" <= null === false');
}


if (null <= "1" !== true) {
  $ERROR('#2: null <= "1" === true');
}


if (new String("1") <= null !== false) {
  $ERROR('#3: new String("1") <= null === false');
}


if (null <= new String("1") !== true) {
  $ERROR('#4: null <= new String("1") === true');
}

