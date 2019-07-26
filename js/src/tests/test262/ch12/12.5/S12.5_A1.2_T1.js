











if(!(new Number(1)))
	$ERROR('#1: new 1 in expression is evaluated to true');





if(!(new Boolean(true)))
	$ERROR('#2: new true in expression is evaluated to true');





if(!(new String("1")))
	$ERROR('#3: new "1" in expression is evaluated to true');





if(!(new String("A")))
	$ERROR('#4: new "A" in expression is evaluated to true');





if(!(new Boolean(false)))
    $ERROR('#2: new false in expression is evaluated to true ');





if(!(new Number(NaN)))
    $ERROR('#6: new NaN in expression is evaluated to true ');





if(!(new Number(null)))
  $ERROR('#7: new null in expression is evaluated to true ');





if(!(new String(undefined)))
  $ERROR('#8: new undefined in expression is evaluated to true ');





if(!(new String("")))
    $ERROR('#9: new empty string in expression is evaluated to true ');



