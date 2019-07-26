











if ((true != {valueOf: function() {return 1}}) !== false) {
  $ERROR('#1: (true != {valueOf: function() {return 1}}) === false');
}


if ((1 != {valueOf: function() {return 1}, toString: function() {return 0}}) !== false) {
  $ERROR('#2: (1 != {valueOf: function() {return 1}, toString: function() {return 0}}) === false');
}


if (("+1" != {valueOf: function() {return 1}, toString: function() {return {}}}) !== false) {
  $ERROR('#3: ("+1" != {valueOf: function() {return 1}, toString: function() {return {}}}) === false');
} 
  

try {
  if ((true != {valueOf: function() {return "+1"}, toString: function() {throw "error"}}) !== false) {
    $ERROR('#4.1: (true != {valueOf: function() {return "+1"}, toString: function() {throw "error"}}) === false');
  }
}
catch (e) {
  if (e === "error") {
    $ERROR('#4.2: (true != {valueOf: function() {return "+1"}, toString: function() {throw "error"}}) not throw "error"');
  } else {
    $ERROR('#4.3: (true != {valueOf: function() {return "+1"}, toString: function() {throw "error"}}) not throw Error. Actual: ' + (e));
  }
}


if ((1 != {toString: function() {return "+1"}}) !== false) {
  $ERROR('#5: (1 != {toString: function() {return "+1"}}) === false');
}


if (("1" != {valueOf: function() {return {}}, toString: function() {return "+1"}}) !== true) {
  $ERROR('#6.1: ("1" != {valueOf: function() {return {}}, toString: function() {return "+1"}}) === true');
} else {
  if (("+1" != {valueOf: function() {return {}}, toString: function() {return "+1"}}) !== false) {
    $ERROR('#6.2: ("+1" != {valueOf: function() {return {}}, toString: function() {return "+1"}}) === false');
  }
}


try {
  (1 != {valueOf: function() {throw "error"}, toString: function() {return 1}});
  $ERROR('#7: (1 != {valueOf: function() {throw "error"}, toString: function() {return 1}}) throw "error"');
}  
catch (e) {
  if (e !== "error") {
    $ERROR('#7: (1 != {valueOf: function() {throw "error"}, toString: function() {return 1}}) throw "error"');
  } 
}


try {
  (1 != {valueOf: function() {return {}}, toString: function() {return {}}});
  $ERROR('#8: (1 != {valueOf: function() {return {}}, toString: function() {return {}}}) throw TypeError');
}  
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#8: (1 != {valueOf: function() {return {}}, toString: function() {return {}}}) throw TypeError');
  } 
}

