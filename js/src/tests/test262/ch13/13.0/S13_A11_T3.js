









function __func(){
    was_del=false;
    for (i=0; i < arguments.length; i++)
       was_del= was_del || delete arguments[i];
    return was_del;
}



if (!__func("A","B",1,2)) {
	$ERROR('#1: Since arguments property has attribute { DontDelete } elements of arguments can be deleted');
}



