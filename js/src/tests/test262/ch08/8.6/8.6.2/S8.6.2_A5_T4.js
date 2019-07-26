










var __count=0;

this["beep"]=function(){__count++};


beep();
if (__count !==1) {
  $ERROR('#1: __count=0; this["beep"]=function(){__count++}; beep(); __count === 1. Actual: ' + (__count));
}





this["beep"]();
if (__count !==2) {
  $ERROR('#2: __count=0; this["beep"]=function(){__count++}; beep(); this["beep"](); __count === 2. Actual: ' + (__count));
}



