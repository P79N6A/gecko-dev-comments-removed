











if (String(new Number()) !== "0") {
  $ERROR('#1: String(new Number()) === "0". Actual: ' + (String(new Number())));
}


if (String(new Number(0)) !== "0") {
  $ERROR('#2: String(new Number(0)) === "0". Actual: ' + (String(new Number(0))));
}


if (String(new Number(Number.NaN)) !== "NaN") {
  $ERROR('#3: String(new Number(Number.NaN)) === Not-a-Number. Actual: ' + (String(new Number(Number.NaN))));
}


if (String(new Number(null)) !== "0") {
  $ERROR('#4: String(new Number(null)) === "0". Actual: ' + (String(new Number(null)))); 
}


if (String(new Number(void 0)) !== "NaN") {
  $ERROR('#5: String(new Number(void 0)) === Not-a-Number. Actual: ' + (String(new Number(void 0))));
}


if (String(new Number(true)) !== "1") {
  $ERROR('#6: String(new Number(true)) === "1". Actual: ' + (String(new Number(true))));
}


if (String(new Number(false)) !== "0") {
  $ERROR('#7: String(new Number(false)) === "0". Actual: ' + (String(new Number(false))));
}


if (String(new Boolean(true)) !== "true") {
  $ERROR('#8: String(new Boolean(true)) === "true". Actual: ' + (String(new Boolean(true))));
}


if (String(new Boolean(false)) !== "false") {
  $ERROR('#9: Number(new Boolean(false)) === "false". Actual: ' + (Number(new Boolean(false))));
}


if (String(new Array(2,4,8,16,32)) !== "2,4,8,16,32") {
  $ERROR('#10: String(new Array(2,4,8,16,32)) === "2,4,8,16,32". Actual: ' + (String(new Array(2,4,8,16,32))));
}


var myobj1 = {
                toNumber : function(){return 12345;}, 
                toString : function(){return 67890;},
                valueOf  : function(){return "[object MyObj]";} 
            };

if (String(myobj1) !== "67890"){
  $ERROR("#11: String(myobj) calls ToPrimitive with hint String");
}


var myobj2 = {
                toNumber : function(){return 12345;},
                toString : function(){return {}}, 
                valueOf  : function(){return "[object MyObj]";} 
            };

if (String(myobj2) !== "[object MyObj]"){
  $ERROR("#12: String(myobj) calls ToPrimitive with hint String");
}


var myobj3 = {
                toNumber : function(){return 12345;},
                valueOf  : function(){return "[object MyObj]";} 
            };

if (String(myobj3) !== "[object Object]"){
  $ERROR("#13: String(myobj) calls ToPrimitive with hint String");
}

