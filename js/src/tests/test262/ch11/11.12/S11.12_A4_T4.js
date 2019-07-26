










if ((true ? undefined : true) !== undefined) {
  $ERROR('#1: (true ? undefined : true) === undefined');
}


if ((true ? null : true) !== null) {
  $ERROR('#2: (true ? null : true) === null');
}

