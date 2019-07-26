










if ((false == undefined) !== false) {
  $ERROR('#1: (false == undefined) === false');
}


if ((Number.NaN == undefined) !== false) {
  $ERROR('#2: (Number.NaN == undefined) === false');
}


if (("undefined" == undefined) !== false) {
  $ERROR('#3: ("undefined" == undefined) === false');
}


if (({} == undefined) !== false) {
  $ERROR('#4: ({} == undefined) === false');
}


if ((false == null) !== false) {
  $ERROR('#5: (false == null) === false');
}


if ((0 == null) !== false) {
  $ERROR('#6: (0 == null) === false');
}


if (("null" == null) !== false) {
  $ERROR('#7: ("null" == null) === false');
}


if (({} == null) !== false) {
  $ERROR('#8: ({} == null) === false');
}

