












x=1;

__evaluated = eval("x+1+x==1");



if (__evaluated !== false) {
	$ERROR('#1: __evaluated === false. Actual:  __evaluated ==='+ __evaluated  );
}



__evaluated = eval("1+1+1==1");



if (__evaluated !== false) {
	$ERROR('#2: __evaluated === false. Actual:  __evaluated ==='+ __evaluated  );
}



