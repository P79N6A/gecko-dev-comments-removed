










if (String.fromCharCode(-32767).charCodeAt(0) !== 32769) {
  $ERROR('#1: String.fromCharCode(-32767).charCodeAt(0) === 32769. Actual: ' + (String.fromCharCode(-32767).charCodeAt(0)));
}


if (String.fromCharCode(-32768).charCodeAt(0) !== 32768) {
  $ERROR('#2: String.fromCharCode(-32768).charCodeAt(0) === 32768. Actual: ' + (String.fromCharCode(-32768).charCodeAt(0)));
}


if (String.fromCharCode(-32769).charCodeAt(0) !== 32767) {
  $ERROR('#3: String.fromCharCode(-32769).charCodeAt(0) === 32767. Actual: ' + (String.fromCharCode(-32769).charCodeAt(0)));
}


if (String.fromCharCode(-65535).charCodeAt(0) !== 1) {
  $ERROR('#4: String.fromCharCode(-65535).charCodeAt(0) === 1. Actual: ' + (String.fromCharCode(-65535).charCodeAt(0)));
}


if (String.fromCharCode(-65536).charCodeAt(0) !== 0) {
  $ERROR('#5: String.fromCharCode(-65536).charCodeAt(0) === 0. Actual: ' + (String.fromCharCode(-65536).charCodeAt(0)));
}


if (String.fromCharCode(-65537).charCodeAt(0) !== 65535) {
  $ERROR('#6: String.fromCharCode(-65537).charCodeAt(0) === 65535. Actual: ' + (String.fromCharCode(-65537).charCodeAt(0)));
}


if (String.fromCharCode(65535).charCodeAt(0) !== 65535) {
  $ERROR('#7: String.fromCharCode(65535).charCodeAt(0) === 65535. Actual: ' + (String.fromCharCode(65535).charCodeAt(0)));
}


if (String.fromCharCode(65536).charCodeAt(0) !== 0) {
  $ERROR('#8: String.fromCharCode(65536).charCodeAt(0) === 0. Actual: ' + (String.fromCharCode(65536).charCodeAt(0)));
}


if (String.fromCharCode(65537).charCodeAt(0) !== 1) {
  $ERROR('#9: String.fromCharCode(65537).charCodeAt(0) === 1. Actual: ' + (String.fromCharCode(65537).charCodeAt(0)));
}


if (String.fromCharCode(131071).charCodeAt(0) !== 65535) {
  $ERROR('#10: String.fromCharCode(131071).charCodeAt(0) === 65535. Actual: ' + (String.fromCharCode(131071).charCodeAt(0)));
}


if (String.fromCharCode(131072).charCodeAt(0) !== 0) {
  $ERROR('#11: String.fromCharCode(131072).charCodeAt(0) === 0. Actual: ' + (String.fromCharCode(131072).charCodeAt(0)));
}


if (String.fromCharCode(131073).charCodeAt(0) !== 1) {
  $ERROR('#12: String.fromCharCode(131073).charCodeAt(0) === 1. Actual: ' + (String.fromCharCode(131073).charCodeAt(0)));
}

