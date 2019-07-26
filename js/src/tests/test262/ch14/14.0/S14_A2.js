











if (typeof f !== 'undefined') {
	$ERROR('#1: typeof f === \'undefined\'. Actual:  typeof f ==='+ typeof f  );
}





if (function f(arg){
	if (arg===0)
	   return 1;
	else
	   return f(arg-1)*arg;
}(3)!==6) {
	$ERROR('#2: FunctionDeclaration cannot be localed inside an Expression');
};



