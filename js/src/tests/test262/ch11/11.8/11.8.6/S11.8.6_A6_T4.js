









MyFunct = function(){};
__my__funct = new MyFunct;



if (!(__my__funct instanceof MyFunct)){
	$ERROR('#1 Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
}


if (__my__funct instanceof Function){
	$ERROR('#2 Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
}


if (!(__my__funct instanceof Object)){
	$ERROR('#3 Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
}


try{
	__my__funct instanceof __my__funct;
	$ERROR('#4 Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
}
catch(e){  
	if (e instanceof TypeError !== true) {
      $ERROR('#4 Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
	}
}

