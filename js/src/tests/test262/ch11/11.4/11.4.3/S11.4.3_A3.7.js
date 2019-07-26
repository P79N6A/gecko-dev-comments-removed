










if (typeof new Function() !== "function") {
  $ERROR('#1: typeof new Function() === "function". Actual: ' + (typeof new Function()));
}


if (typeof Function() !== "function") {
  $ERROR('#2: typeof Function() === "function". Actual: ' + (typeof Function()));
}


if (typeof Object !== "function") {
  $ERROR('#3: typeof Object === "function". Actual: ' + (typeof Object));
}


if (typeof String !== "function") {
  $ERROR('#4: typeof String === "function". Actual: ' + (typeof String));
}


if (typeof Boolean !== "function") {
  $ERROR('#5: typeof Boolean === "function". Actual: ' + (typeof Boolean));
}


if (typeof Number !== "function") {
  $ERROR('#6: typeof Number === "function". Actual: ' + (typeof Number));
}


if (typeof Date !== "function") {
  $ERROR('#7: typeof Date === "function". Actual: ' + (typeof Date));
}


if (typeof Error !== "function") {
  $ERROR('#8: typeof Error === "function". Actual: ' + (typeof Error));
}


if (typeof RegExp !== "function") {
  $ERROR('#9: typeof RegExp === "function". Actual: ' + (typeof RegExp));
}

