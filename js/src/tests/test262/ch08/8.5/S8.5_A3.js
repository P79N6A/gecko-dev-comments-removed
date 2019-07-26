









var x=NaN;



if (typeof(x) !== "number"){
  $ERROR('#1: var x=NaN; typeof(x) === "number". Actual: ' + (typeof(x)));
}





if (typeof(NaN) !== "number"){
  $ERROR('#2: typeof(NaN) === "number". Actual: ' + (typeof(NaN)));
}



