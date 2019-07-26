









var str="";
var strObj=new String("");
var strObj_=new String();



if (str.constructor !== strObj.constructor){
  $ERROR('#1: "".constructor === new String("").constructor');
}





if (str.constructor !== strObj_.constructor){
  $ERROR('#2: "".constructor === new String().constructor');
}





if (str != strObj){
  $ERROR('#3: values of str=""; and strObj=new String(""); are equal');
}





if (str === strObj){
  $ERROR('#4: objects of str=""; and strObj=new String(""); are different');
}





if (str != strObj_){
  $ERROR('#5: values of str=""; and strObj=new String(); are equal');
}





if (str === strObj_){
  $ERROR('#6: objects of str=""; and strObj=new String(); are different');
}



