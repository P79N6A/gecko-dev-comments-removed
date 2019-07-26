










try {
  "toString" in true;
  $ERROR('#1: "toString" in true throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: "toString" in true throw TypeError');  
  }
}


try {
  "MAX_VALUE" in 1;
  $ERROR('#2: "MAX_VALUE" in 1 throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: "MAX_VALUE" in 1 throw TypeError');  
  }
}


try {
  "length" in "string";
  $ERROR('#3: "length" in "string" throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: "length" in "string" throw TypeError');  
  }
}


try {
  "toString" in undefined;
  $ERROR('#4: "toString" in undefined throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#4: "toString" in undefined throw TypeError');  
  }
}


try {
  "toString" in null;
  $ERROR('#5: "toString" in null throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#5: "toString" in null throw TypeError');  
  }
}

