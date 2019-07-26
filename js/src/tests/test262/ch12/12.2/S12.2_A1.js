













try {
	__x = __x;
    __y = __x ? "good fellow" : "liar"; 
    __z = __z === __x ? 1 : 0; 
} catch (e) {
	$ERROR('#1: Using declarated variable before it declaration is admitted');
}





try{
    __something__undefined = __something__undefined;
    $ERROR('#2: "__something__undefined = __something__undefined" lead to throwing exception');
} catch(e){
    $PRINT(e.message);
}





if ((__y !== "liar")&(__z !== 1)) {
	$ERROR('#3: (__y === "liar") and (__z === 1). Actual:  __y ==='+__y+' and __z ==='+__z  );
}



var __x, __y = true, __z = __y ? "smeagol" : "golum";



if (!__y&!(__z = "smeagol")) {
	$ERROR('#4: A variable with an Initialiser is assigned the value of its AssignmentExpression when the VariableStatement is executed');
}



