










if (String.fromCharCode(new Boolean(true)).charCodeAt(0) !== 1) {
  $ERROR('#1: String.fromCharCode(new Boolean(true)).charCodeAt(0) === 1. Actual: ' + (String.fromCharCode(new Boolean(true)).charCodeAt(0)));
}


if (String.fromCharCode(false).charCodeAt(0) !== 0) {
  $ERROR('#2: String.fromCharCode(false).charCodeAt(0) === 0. Actual: ' + (String.fromCharCode(false).charCodeAt(0)));
}

