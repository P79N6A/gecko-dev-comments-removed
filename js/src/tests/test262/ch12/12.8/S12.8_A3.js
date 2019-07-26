









LABEL_OUT : var x=0, y=0;

LABEL_DO_LOOP : do {
    LABEL_IN : x=2;
    break ;
    LABEL_IN_2 : var y=2;
    
    function IN_DO_FUNC(){}
} while(0);

LABEL_ANOTHER_LOOP : do {
    ;
} while(0);

function OUT_FUNC(){}



if ((x!==2)&&(y!==0)) {
	$ERROR('#1: x === 2 and y === 0. Actual:  x ==='+x+' and y ==='+y);
}



