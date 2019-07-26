











var str="abcdfg";
if (typeof(str)!=="string"){
  $ERROR('#1: var str="abcdfg"; typeof(str) === "string". Actual: ' + (typeof(str)));
}





var str2='qwerty';
if (typeof(str2)!=="string"){
  $ERROR('#2: var str2=\'qwerty\'; typeof(str) === "string". Actual: ' + (typeof(str2)));
}





var __str='\u0042\u0043\u0044\u0045\u0046\u0047\u0048';
if (typeof(__str)!=="string"){
  $ERROR('#3: var __str=\'\\u0042\\u0043\\u0044\\u0045\\u0046\\u0047\\u0048\'; typeof(__str) === "string". Actual: ' + (typeof(__str)));
}





var str__="\u0042\u0043\u0044\u0045\u0046\u0047\u0048";
if (typeof(str__)!=="string"){
  $ERROR('#4: var str__="abcdfg"; typeof(str__) === "string". Actual: ' + (typeof(str__)));
}



