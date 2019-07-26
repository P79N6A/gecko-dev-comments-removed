









__str=""

for(var index=0; index<10; index+=1) {
	if (index>5)break;
	__str+=index;
}

if (__str!=="012345") {
	$ERROR('#1: __str === "012345". Actual:  __str ==='+ __str  );
}

