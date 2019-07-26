










if ((false != undefined) !== true) {
  $ERROR('#1: (false != undefined) === true');
}


if ((Number.NaN != undefined) !== true) {
  $ERROR('#2: (Number.NaN != undefined) === true');
}


if (("undefined" != undefined) !== true) {
  $ERROR('#3: ("undefined" != undefined) === true');
}


if (({} != undefined) !== true) {
  $ERROR('#4: ({} != undefined) === true');
}


if ((false != null) !== true) {
  $ERROR('#5: (false != null) === true');
}


if ((0 != null) !== true) {
  $ERROR('#6: (0 != null) === true');
}


if (("null" != null) !== true) {
  $ERROR('#7: ("null" != null) === true');
}


if (({} != null) !== true) {
  $ERROR('#8: ({} != null) === true');
}

