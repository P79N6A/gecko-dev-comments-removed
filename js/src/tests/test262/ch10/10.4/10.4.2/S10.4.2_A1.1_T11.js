










var i;
var j;
str1 = '';
str2 = '';

for(i in this){
  str1+=i;
}

eval('for(j in this){\nstr2+=j;\n}');

if(!(str1 === str2)){
  $ERROR("#1: scope chain must contain same objects in the same order as the calling context");
}

this.x = 1;
this.y = 2;

