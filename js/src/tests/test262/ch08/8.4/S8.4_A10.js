











var __str = "\u0041A\u0042B\u0043C";
if (__str !== 'AABBCC'){
  $ERROR('#1: var __str = "\\u0041A\\u0042B\\u0043C"; __str === \'AABBCC\'. Actual: ' + (__str));
};





var __str__ = "\u0041\u0042\u0043"+'ABC';
if (__str__ !== 'ABCABC'){
  $ERROR('#2: var __str__ = "\\u0041\\u0042\\u0043"+\'ABC\'; __str__ === \'ABCABC\'. Actual: ' + (__str__));
};





var str__ = "ABC"+'\u0041\u0042\u0043';
if (str__ !== "ABCABC"){
  $ERROR('#2: var str__ = "ABC"+\'\\u0041\\u0042\\u0043\'; str__ === "ABCABC". Actual: ' + (str__));
};



