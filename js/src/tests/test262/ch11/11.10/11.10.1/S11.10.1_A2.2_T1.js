










if (({valueOf: function() {return 1}} & 1) !== 1) {
  $ERROR('#1: ({valueOf: function() {return 1}} & 1) === 1. Actual: ' + (({valueOf: function() {return 1}} & 1)));
}


if (({valueOf: function() {return 1}, toString: function() {return 0}} & 1) !== 1) {
  $ERROR('#2: ({valueOf: function() {return 1}, toString: function() {return 0}} & 1) === 1. Actual: ' + (({valueOf: function() {return 1}, toString: function() {return 0}} & 1)));
}


if (({valueOf: function() {return 1}, toString: function() {return {}}} & 1) !== 1) {
  $ERROR('#3: ({valueOf: function() {return 1}, toString: function() {return {}}} & 1) === 1. Actual: ' + (({valueOf: function() {return 1}, toString: function() {return {}}} & 1)));
}


try {
  if (({valueOf: function() {return 1}, toString: function() {throw "error"}} & 1) !== 1) {
    $ERROR('#4.1: ({valueOf: function() {return 1}, toString: function() {throw "error"}} & 1) === 1. Actual: ' + (({valueOf: function() {return 1}, toString: function() {throw "error"}} & 1)));
  }
}
catch (e) {
  if (e === "error") {
    $ERROR('#4.2: ({valueOf: function() {return 1}, toString: function() {throw "error"}} & 1) not throw "error"');
  } else {
    $ERROR('#4.3: ({valueOf: function() {return 1}, toString: function() {throw "error"}} & 1) not throw Error. Actual: ' + (e));
  }
}


if ((1 & {toString: function() {return 1}}) !== 1) {
  $ERROR('#5.1: (1 & {toString: function() {return 1}}) === 1. Actual: ' + ((1 & {toString: function() {return 1}})));
}


if ((1 & {valueOf: function() {return {}}, toString: function() {return 1}}) !== 1) {
  $ERROR('#6: (1 & {valueOf: function() {return {}}, toString: function() {return 1}}) === 1. Actual: ' + ((1 & {valueOf: function() {return {}}, toString: function() {return 1}})));
}


try {
  1 & {valueOf: function() {throw "error"}, toString: function() {return 1}};
  $ERROR('#7.1: 1 & {valueOf: function() {throw "error"}, toString: function() {return 1}} throw "error". Actual: ' + (1 & {valueOf: function() {throw "error"}, toString: function() {return 1}}));
}  
catch (e) {
  if (e !== "error") {
    $ERROR('#7.2: 1 & {valueOf: function() {throw "error"}, toString: function() {return 1}} throw "error". Actual: ' + (e));
  } 
}


try {
  1 & {valueOf: function() {return {}}, toString: function() {return {}}};
  $ERROR('#8.1: 1 & {valueOf: function() {return {}}, toString: function() {return {}}} throw TypeError. Actual: ' + (1 & {valueOf: function() {return {}}, toString: function() {return {}}}));
}  
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#8.2: 1 & {valueOf: function() {return {}}, toString: function() {return {}}} throw TypeError. Actual: ' + (e));
  } 
}

