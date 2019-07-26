











try{
	x=x;
	$ERROR('#1: "x=x" lead to throwing exception');
}catch(e){
	$PRINT(e.message);
};



eval("var x");



try{
	x=x;
}catch(e){
	$ERROR('#2: VariableDeclaration inside Eval statement is initialized when program reaches the eval statement '+e.message);
};



