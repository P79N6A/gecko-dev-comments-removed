










if (typeof 0 !== "number") {
  $ERROR('#1: typeof 0 === "number". Actual: ' + (typeof 0));
}


var x = 0;
if (typeof x !== "number") {
  $ERROR('#2: typeof x === "number". Actual: ' + (typeof x));
}


var x = new Object();
if (typeof x !== "object") {
  $ERROR('#3: var x = new Object(); typeof x === "object". Actual: ' + (typeof x));
}

