










if (1..toString() !== "1") {
  $ERROR('#1: 1..toString() === "1". Actual: ' + (1..toString()));
}


if (1.1.toFixed(5) !== "1.10000") {
  $ERROR('#2: 1.1.toFixed(5) === "1.10000". Actual: ' + (1.1.toFixed(5)));
}


if (1["toString"]() !== "1") {
  $ERROR('#3: 1["toString"]() === "1". Actual: ' + (1["toString"]()));
}


if (1.["toFixed"](5) !== "1.00000") {
  $ERROR('#4: 1.["toFixed"](5) === "1.00000". Actual: ' + (1.["toFixed"](5)));
}


if (new Number(1).toString() !== "1") {
  $ERROR('#5: new Number(1).toString() === "1". Actual: ' + (new Number(1).toString()));
}


if (new Number(1)["toFixed"](5) !== "1.00000") {
  $ERROR('#6: new Number(1)["toFixed"](5) === "1.00000". Actual: ' + (new Number(1)["toFixed"](5)));
} 

