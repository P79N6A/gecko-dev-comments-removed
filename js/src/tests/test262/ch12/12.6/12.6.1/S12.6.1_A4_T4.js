









do_out : do {
    var __in__do__before__break="reached";
    do_in : do {
        var __in__do__IN__before__break="reached";
        break;
        var __in__do__IN__after__break="where am i";
    } while (0);
    var __in__do__after__break="where am i";
} while(2===1);



if (!(__in__do__before__break&&__in__do__IN__before__break&&!__in__do__IN__after__break&&__in__do__after__break)) {
	$ERROR('#1: (__in__do__before__break&&__in__do__IN__before__break&&!__in__do__IN__after__break&&__in__do__after__break)===true. Actual:  (__in__do__before__break&&__in__do__IN__before__break&&!__in__do__IN__after__break&&__in__do__after__break)==='+ (__in__do__before__break&&__in__do__IN__before__break&&!__in__do__IN__after__break&&__in__do__after__break) );
}




