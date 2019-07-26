









var x;



if (!(x === undefined)) {
  $ERROR('#1: var x; x === undefined. Actual: ' + (x));
}





if (!(typeof(x) === "undefined")) {
  $ERROR('#2: var x; typeof(x) === "undefined". Actual: ' + (typeof(x)));
}





if (!(x === void 0)) {
  $ERROR('#3: var x; x === void 0. Actual: ' + (x));
}



