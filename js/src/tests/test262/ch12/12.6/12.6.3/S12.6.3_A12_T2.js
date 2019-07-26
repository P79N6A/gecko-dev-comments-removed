









__str="";

outer : for(index=0; index<4; index+=1) {
    nested : for(index_n=0; index_n<=index; index_n++) {
	if (index*index_n >= 4)break nested;
	__str+=""+index+index_n;
    } 
}



if (__str !== "00101120213031") {
	$ERROR('#1: __str === "00101120213031". Actual:  __str ==='+ __str  );
}



__str="";

outer : for(index=0; index<4; index+=1) {
    nested : for(index_n=0; index_n<=index; index_n++) {
	if (index*index_n >= 4)break outer;
	__str+=""+index+index_n;
    } 
}



if (__str !== "0010112021") {
	$ERROR('#2: __str === "0010112021". Actual:  __str ==='+ __str  );
}



__str="";

outer : for(index=0; index<4; index+=1) {
    nested : for(index_n=0; index_n<=index; index_n++) {
	if (index*index_n >= 4)break ;
	__str+=""+index+index_n;
    } 
}



if (__str !== "00101120213031") {
	$ERROR('#3: __str === "00101120213031". Actual:  __str ==='+ __str  );
}






