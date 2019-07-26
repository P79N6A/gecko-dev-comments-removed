










var date = new Date();
if (date + date !== date.toString() + date.toString()) {
  $ERROR('#1: var date = new Date(); date + date === date.toString() + date.toString(). Actual: ' + (date + date));  
}


var date = new Date();
if (date + 0 !== date.toString() + "0") {
  $ERROR('#2: var date = new Date(); date + 0 === date.toString() + "0". Actual: ' + (date + 0));  
}


var date = new Date();
if (date + true !== date.toString() + "true") {
  $ERROR('#3: var date = new Date(); date + true === date.toString() + "true". Actual: ' + (date + true));  
}


var date = new Date();
if (date + new Object() !== date.toString() + "[object Object]") {
  $ERROR('#4: var date = new Date(); date + new Object() === date.toString() + "[object Object]". Actual: ' + (date + new Object()));  
}


