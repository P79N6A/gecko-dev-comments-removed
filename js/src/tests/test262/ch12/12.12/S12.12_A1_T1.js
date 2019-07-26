










var object = {p1: 1, p2: 1};
var result = 0;
lbl: for(var i in object){
  result += object[i];
  break lbl;
}

if(!(result === 1)){
  $ERROR("'break label' should break execution of labelled iteration statement");
}

