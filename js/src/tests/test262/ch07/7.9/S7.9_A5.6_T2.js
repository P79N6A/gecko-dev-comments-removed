










var x=0, y=2;

x
--
y



if ((x!==0)&(y!==1)) {
	$ERROR('#1: Check Postfix Increment Operator for automatic semicolon insertion');
}



x
--y



if ((x!==0)&(y!==0)) {
	$ERROR('#2: Check Postfix Increment Operator for automatic semicolon insertion');
}




