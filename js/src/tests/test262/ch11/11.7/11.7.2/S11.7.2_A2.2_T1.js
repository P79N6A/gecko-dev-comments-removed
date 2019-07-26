










if ({valueOf: function() {return -4}} >> 1 !== -2) {
  $ERROR('#1: {valueOf: function() {return -4}} >> 1 === -2. Actual: ' + ({valueOf: function() {return -4}} >> 1));
}


if ({valueOf: function() {return -4}, toString: function() {return 0}} >> 1 !== -2) {
  $ERROR('#2: {valueOf: function() {return -4}, toString: function() {return 0}} >> 1 === -2. Actual: ' + ({valueOf: function() {return -4}, toString: function() {return 0}} >> 1));
}


if ({valueOf: function() {return -4}, toString: function() {return {}}} >> 1 !== -2) {
  $ERROR('#3: {valueOf: function() {return -4}, toString: function() {return {}}} >> 1 === -2. Actual: ' + ({valueOf: function() {return -4}, toString: function() {return {}}} >> 1));
}


try {
  if ({valueOf: function() {return -4}, toString: function() {throw "error"}} >> 1 !== -2) {
    $ERROR('#4.1: {valueOf: function() {return -4}, toString: function() {throw "error"}} >> 1 === -2. Actual: ' + ({valueOf: function() {return -4}, toString: function() {throw "error"}} >> 1));
  }
}
catch (e) {
  if (e === "error") {
    $ERROR('#4.2: {valueOf: function() {return -4}, toString: function() {throw "error"}} >> 1 not throw "error"');
  } else {
    $ERROR('#4.3: {valueOf: function() {return -4}, toString: function() {throw "error"}} >> 1 not throw Error. Actual: ' + (e));
  }
}


if (-4 >> {toString: function() {return 1}} !== -2) {
  $ERROR('#5: -4 >> {toString: function() {return 1}} === -2. Actual: ' + (-4 >> {toString: function() {return 1}}));
}


if (-4 >> {valueOf: function() {return {}}, toString: function() {return 1}} !== -2) {
  $ERROR('#6: -4 >> {valueOf: function() {return {}}, toString: function() {return 1}} === -2. Actual: ' + (-4 >> {valueOf: function() {return {}}, toString: function() {return 1}}));
}


try {
  -4 >> {valueOf: function() {throw "error"}, toString: function() {return 1}};
  $ERROR('#7.1: -4 >> {valueOf: function() {throw "error"}, toString: function() {return 1}} throw "error". Actual: ' + (-4 >> {valueOf: function() {throw "error"}, toString: function() {return 1}}));
}  
catch (e) {
  if (e !== "error") {
    $ERROR('#7.2: -4 >> {valueOf: function() {throw "error"}, toString: function() {return 1}} throw "error". Actual: ' + (e));
  } 
}


try {
  -4 >> {valueOf: function() {return {}}, toString: function() {return {}}};
  $ERROR('#8.1: -4 >> {valueOf: function() {return {}}, toString: function() {return {}}} throw TypeError. Actual: ' + (-4 >> {valueOf: function() {return {}}, toString: function() {return {}}}));
}  
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#8.2: -4 >> {valueOf: function() {return {}}, toString: function() {return {}}} throw TypeError. Actual: ' + (e));
  } 
}

