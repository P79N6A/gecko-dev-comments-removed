










if ((false ? true : undefined) !== undefined) {
  $ERROR('#1: (false ? true : undefined) === undefined');
}


if ((false ? true : null) !== null) {
  $ERROR('#2: (false ? true : null) === null');
}

