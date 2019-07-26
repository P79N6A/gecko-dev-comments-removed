










if (String.fromCharCode(new String(1)).charCodeAt(0) !== 1) {
  $ERROR('#1: String.fromCharCode(new String(1)).charCodeAt(0) === 1. Actual: ' + (String.fromCharCode(new String(1)).charCodeAt(0)));
}


if (String.fromCharCode("-1.234").charCodeAt(0) !== 65535) {
  $ERROR('#2: String.fromCharCode("-1.234").charCodeAt(0) === 65535. Actual: ' + (String.fromCharCode("-1.234").charCodeAt(0)));
}

