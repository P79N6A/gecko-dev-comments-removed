










function f(){
  var i;
  var j;
  str1 = '';
  str2 = '';
  var x = 1;
  var y = 2;
  
  for(i in this){
    str1+=i;
  }
  
  eval('for(j in this){\nstr2+=j;\n}');

  return (str1 === str2); 
}

if(!f()){
  $ERROR("#1: scope chain must contain same objects in the same order as the calling context");
}

