









__str="";

__evaluated = eval("for(var ind in (hash={2:'b',1:'a',4:'d',3:'c'}))__str+=hash[ind]");



if ( !( (__evaluated.indexOf("a")!==-1)& (__evaluated.indexOf("b")!==-1)& (__evaluated.indexOf("c")!==-1)&(__evaluated.indexOf("d")!==-1) ) ) {
	$ERROR('#1: (__evaluated.indexOf("a")!==-1)& (__evaluated.indexOf("b")!==-1)& (__evaluated.indexOf("c")!==-1)&(__evaluated.indexOf("d")!==-1)');
}





if (__str !== __evaluated) {
	$ERROR('#2: __str === __evaluated. Actual:  __str ==='+ __str  );
}







