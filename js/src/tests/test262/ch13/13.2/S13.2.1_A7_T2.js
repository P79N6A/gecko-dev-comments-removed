











var __func = function (){
    var x = null;
    return x;
}



try{
    var x=__func();
} catch(e){
    $ERROR('#1: var x=__func() does not lead to throwing exception. Actual: exception is '+e);
}



