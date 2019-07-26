










try{
	({}) instanceof this;
	$ERROR('#1: Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
}
catch(e){
  if (e instanceof TypeError !== true) {
    $ERROR('#1: Only Function objects implement [[HasInstance]] and consequently can be proper ShiftExpression for The instanceof operator');
  }
}

