










var object = {undefined : true};
if (object.undefined !== true) {
  $ERROR('#1: var object = {undefined : true}; object.undefined === true');
}


var object = {undefined : true};
if (object["undefined"] !== true) {
  $ERROR('#2: var object = {undefined : true}; object["undefined"] === true');
}


var object = {"true" : true};
if (object["true"] !== true) {
  $ERROR('#3: var object = {"true" : true}; object["true"] === true');
}


var object = {"null" : true};
if (object["null"] !== true) {
  $ERROR('#4: var object = {"null" : true}; object["null"] === true');
}

