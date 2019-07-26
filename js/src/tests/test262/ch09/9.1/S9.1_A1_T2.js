










var object = {valueOf: function() {return 0}, toString: function() {return 1}};
if (String(object) !== "1") {
  $ERROR('#1: var object = {valueOf: function() {return 0}, toString: function() {return 1}}; String(object) === "1". Actual: ' + (String(object)));
}


var object = {valueOf: function() {return 0}, toString: function() {return {}}};
if (String(object) !== "0") {
  $ERROR('#2: var object = {valueOf: function() {return 0}, toString: function() {return {}}}; String(object) === "0". Actual: ' + (String(object)));
}


