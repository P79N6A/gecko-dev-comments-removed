









do_out : while(1===1) {
    if (__in__do__before__break) break;
    var __in__do__before__break="once";
    do_in : while (1) {
        var __in__do__IN__before__break="in";
        break do_out;
        var __in__do__IN__after__break="the";
    } ;
    var __in__do__after__break="lifetime";
} ;



if (!(__in__do__before__break&&__in__do__IN__before__break&&!__in__do__IN__after__break&&!__in__do__after__break)) {
	$ERROR('#1: Break inside do-while is allowed as its described at standard');
}



