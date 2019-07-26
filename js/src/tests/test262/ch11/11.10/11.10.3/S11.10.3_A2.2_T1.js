










if (({valueOf: function() {return 1}} | 0) !== 1) {
  $ERROR('#1: ({valueOf: function() {return 1}} | 0) === 1. Actual: ' + (({valueOf: function() {return 1}} | 0)));
}


if (({valueOf: function() {return 1}, toString: function() {return 0}} | 0) !== 1) {
  $ERROR('#2: ({valueOf: function() {return 1}, toString: function() {return 0}} | 0) === 1. Actual: ' + (({valueOf: function() {return 1}, toString: function() {return 0}} | 0)));
}


if (({valueOf: function() {return 1}, toString: function() {return {}}} | 0) !== 1) {
  $ERROR('#3: ({valueOf: function() {return 1}, toString: function() {return {}}} | 0) === 1. Actual: ' + (({valueOf: function() {return 1}, toString: function() {return {}}} | 0)));
}


try {
  if (({valueOf: function() {return 1}, toString: function() {throw "error"}} | 0) !== 1) {
    $ERROR('#4.1: ({valueOf: function() {return 1}, toString: function() {throw "error"}} | 0) === 1. Actual: ' + (({valueOf: function() {return 1}, toString: function() {throw "error"}} | 0)));
  }
}
catch (e) {
  if (e === "error") {
    $ERROR('#4.2: ({valueOf: function() {return 1}, toString: function() {throw "error"}} | 0) not throw "error"');
  } else {
    $ERROR('#4.3: ({valueOf: function() {return 1}, toString: function() {throw "error"}} | 0) not throw Error. Actual: ' + (e));
  }
}


if ((0 | {toString: function() {return 1}}) !== 1) {
  $ERROR('#5: (0 | {toString: function() {return 1}}) === 1. Actual: ' + ((0 | {toString: function() {return 1}})));
}


if ((0 | {valueOf: function() {return {}}, toString: function() {return 1}}) !== 1) {
  $ERROR('#6: (0 | {valueOf: function() {return {}}, toString: function() {return 1}}) === 1. Actual: ' + ((0 | {valueOf: function() {return {}}, toString: function() {return 1}})));
}


try {
  0 | {valueOf: function() {throw "error"}, toString: function() {return 1}};
  $ERROR('#7.1: 0 | {valueOf: function() {throw "error"}, toString: function() {return 1}} throw "error". Actual: ' + (0 | {valueOf: function() {throw "error"}, toString: function() {return 1}}));
}  
catch (e) {
  if (e !== "error") {
    $ERROR('#7.2: 0 | {valueOf: function() {throw "error"}, toString: function() {return 1}} throw "error". Actual: ' + (e));
  } 
}


try {
  0 | {valueOf: function() {return {}}, toString: function() {return {}}};
  $ERROR('#8.1: 0 | {valueOf: function() {return {}}, toString: function() {return {}}} throw TypeError. Actual: ' + (0 | {valueOf: function() {return {}}, toString: function() {return {}}}));
}  
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#8.2: 0 | {valueOf: function() {return {}}, toString: function() {return {}}} throw TypeError. Actual: ' + (e));
  } 
}

