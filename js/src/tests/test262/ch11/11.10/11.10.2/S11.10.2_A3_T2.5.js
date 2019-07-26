










if ((true ^ "1") !== 0) {
  $ERROR('#1: (true ^ "1") === 0. Actual: ' + ((true ^ "1")));
}


if (("1" ^ true) !== 0) {
  $ERROR('#2: ("1" ^ true) === 0. Actual: ' + (("1" ^ true)));
}


if ((new Boolean(true) ^ "1") !== 0) {
  $ERROR('#3: (new Boolean(true) ^ "1") === 0. Actual: ' + ((new Boolean(true) ^ "1")));
}


if (("1" ^ new Boolean(true)) !== 0) {
  $ERROR('#4: ("1" ^ new Boolean(true)) === 0. Actual: ' + (("1" ^ new Boolean(true))));
}


if ((true ^ new String("1")) !== 0) {
  $ERROR('#5: (true ^ new String("1")) === 0. Actual: ' + ((true ^ new String("1"))));
}


if ((new String("1") ^ true) !== 0) {
  $ERROR('#6: (new String("1") ^ true) === 0. Actual: ' + ((new String("1") ^ true)));
}


if ((new Boolean(true) ^ new String("1")) !== 0) {
  $ERROR('#7: (new Boolean(true) ^ new String("1")) === 0. Actual: ' + ((new Boolean(true) ^ new String("1"))));
}


if ((new String("1") ^ new Boolean(true)) !== 0) {
  $ERROR('#8: (new String("1") ^ new Boolean(true)) === 0. Actual: ' + ((new String("1") ^ new Boolean(true))));
}

