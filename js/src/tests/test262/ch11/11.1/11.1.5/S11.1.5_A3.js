









var object = {0 : 1, "1" : "x", o : {}};


if (object[0] !== 1) {
  $ERROR('#1: var object = {0 : 1; "1" : "x"; o : {}}; object[0] === 1. Actual: ' + (object[0]));
}


if (object["1"] !== "x") {
  $ERROR('#2: var object = {0 : 1; "1" : "x"; o : {}}; object["1"] === "x". Actual: ' + (object["1"]));
}


if (typeof object.o !== "object") {
  $ERROR('#1: var object = {0 : 1; "1" : "x"; o : {}}; typeof object.o === "object". Actual: ' + (typeof object.o));
}

