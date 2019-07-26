










if (String.fromCharCode(1.2345).charCodeAt(0) !== 1) {
  $ERROR('#1: String.fromCharCode(1.2345).charCodeAt(0) === 1. Actual: ' + (String.fromCharCode(1.2345).charCodeAt(0)));
}


if (String.fromCharCode(-5.4321).charCodeAt(0) !== 65531) {
  $ERROR('#2: String.fromCharCode(-5.4321).charCodeAt(0) === 65531. Actual: ' + (String.fromCharCode(-5.4321).charCodeAt(0)));
}

