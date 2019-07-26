









var x=+Infinity;



if (typeof(x) !== "number"){
  $ERROR('#1: var x=+Infinity; typeof(x) === "number". Actual: ' + (typeof(x)));
}





if (typeof(+Infinity) !== "number"){
  $ERROR('#2: typeof(+Infinity) === "number". Actual: ' + (typeof(+Infinity)));
}



