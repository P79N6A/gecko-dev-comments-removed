












x="5+1|0===0";

__evaluated = eval(x);



if (__evaluated !== 7) {
	$ERROR('#1: __evaluated === 7. Actual:  __evaluated ==='+ __evaluated  );
}



__evaluated = eval("2*"+x+">-1");



if (__evaluated !== 11) {
	$ERROR('#2: __evaluated === 11. Actual:  __evaluated ==='+ __evaluated  );
}



