










var object = {valueOf: function() {return -2}, toString: function() {return "-2"}};
if ("-1" < object) {
  $ERROR('#1: var object = {valueOf: function() {return -2}, toString: function() {return "-2"}}; "-1" < object');
}


var object = {valueOf: function() {return "-2"}, toString: function() {return -2}};
if (object < "-1") {
  $ERROR('#2: var object = {valueOf: function() {return "-2"}, toString: function() {return -2}}; object < "-1"');
}


