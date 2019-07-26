









var str="";
var strObj=new String;



if (str.constructor !== strObj.constructor){
  $ERROR('#1: "".constructor === new String.constructor');
}





if (str != strObj){
  $ERROR('#2: values of str=""; and strObj=new String(""); are equal');
}





if (str === strObj){
  $ERROR('#3: objects of str=""; and strObj=new String(""); are different');
}





if (typeof str == typeof strObj){
  $ERROR('#4: types of str=""; and strObj=new String(""); are different');
}




