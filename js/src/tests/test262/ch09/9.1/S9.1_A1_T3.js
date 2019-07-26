










var object = {valueOf: function() {return 1}, toString: function() {return 0}};
if (object + "" !== "1") {
  $ERROR('#1: var object = {valueOf: function() {return 1}, toString: function() {return 0}}; object + "" === "1". Actual: ' + (object + ""));
}


var object = {valueOf: function() {return "1"}, toString: function() {return 0}};
if (object + 0 !== "10") {
  $ERROR('#2: var object = {valueOf: function() {return "1"}, toString: function() {return 0}}; object + 0 === "10". Actual: ' + (object + 0));
}


