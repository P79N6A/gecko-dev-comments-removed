









function __func(){ return delete arguments;}



if (__func("A","B",1,2)) {
	$ERROR('#1: arguments property has attribute { DontDelete }');
}



