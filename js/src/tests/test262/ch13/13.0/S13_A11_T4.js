









function __func(){
    is_undef=true;
    for (i=0; i < arguments.length; i++)
    {
        delete arguments[i];
        is_undef= is_undef && (typeof arguments[i] === "undefined");
    };       
    return is_undef;
};



if (!__func("A","B",1,2)) {
	$ERROR('#1: Since arguments property has attribute { DontDelete }, but elements of arguments can be deleted');
}



