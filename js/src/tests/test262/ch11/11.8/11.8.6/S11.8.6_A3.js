










try {
  true instanceof true;
  $ERROR('#1: true instanceof true throw TypeError');  
}
catch (e) {
  if (e instanceof TypeError !== true) {
    $ERROR('#1: true instanceof true throw TypeError');  
  }
}


try {
  1 instanceof 1;
  $ERROR('#2: 1 instanceof 1 throw TypeError');  
}
catch (e) {
  if (e instanceof TypeError !== true) {
    $ERROR('#2: 1 instanceof 1 throw TypeError');  
  }
}


try {
  "string" instanceof "string";
  $ERROR('#3: "string" instanceof "string" throw TypeError');  
}
catch (e) {
  if (e instanceof TypeError !== true) {
    $ERROR('#3: "string" instanceof "string" throw TypeError');  
  }
}


try {
  undefined instanceof undefined;
  $ERROR('#4: undefined instanceof undefined throw TypeError');  
}
catch (e) {
  if (e instanceof TypeError !== true) {
    $ERROR('#4: undefined instanceof undefined throw TypeError');  
  }
}


try {
  null instanceof null;
  $ERROR('#5: null instanceof null throw TypeError');  
}
catch (e) {
  if (e instanceof TypeError !== true) {
    $ERROR('#5: null instanceof null throw TypeError');  
  }
}

