












contractid_prefix = "@mozilla.org/";
contractid_suffix = ";1";


var iface_test = Components.interfaces.nsISupports;

var i_str = iface_test.number


var same = Object;










var data = [
 ["supports-id"      ,"nsISupportsID"      ,iface_test,i_str ,eqfn  ,  null],
 ["supports-cstring"  ,"nsISupportsCString"  ,"foo"     ,same  ,null  ,  null],
 ["supports-string" ,"nsISupportsString" ,"bar"     ,same  ,null  ,  null],
 ["supports-PRBool"  ,"nsISupportsPRBool"  ,true      ,same  ,null  ,  null],
 ["supports-PRBool"  ,"nsISupportsPRBool"  ,false     ,same  ,null  ,  null],
 ["supports-PRUint8" ,"nsISupportsPRUint8" ,7         ,same  ,null  ,  null],
 ["supports-PRUint16","nsISupportsPRUint16",12345     ,same  ,null  ,  null],
 ["supports-PRUint32","nsISupportsPRUint32",123456    ,same  ,null  ,  null],
 ["supports-PRUint64","nsISupportsPRUint64",1234567   ,same  ,null  ,  null],
 ["supports-PRTime"  ,"nsISupportsPRTime"  ,12345678  ,same  ,null  ,  null],
 ["supports-char"    ,"nsISupportsChar"    ,'z'       ,same  ,null  ,  null],
 ["supports-PRInt16" ,"nsISupportsPRInt16" ,-123      ,same  ,null  ,  null],
 ["supports-PRInt32" ,"nsISupportsPRInt32" ,-3456     ,same  ,null  ,  null],
 ["supports-PRInt64" ,"nsISupportsPRInt64" ,-1234566  ,same  ,null  ,  null],
 ["supports-float"   ,"nsISupportsFloat"   , 12.0001  ,same  ,fcmp  ,  fcmp],
 ["supports-double"  ,"nsISupportsDouble"  , 1.0029202,same  ,fcmp  ,  fcmp],
];

function println(s)
{
    if(this.document === undefined)
        print(s);
    else
        dump(s+"\n");
}


function fcmp(v1, v2)
{
    var f1 = parseFloat(v1);
    var f2 = parseFloat(v2);
    var retval = ((f1 - f2) < 0.001) || ((f2 - f1) < 0.001);

    return retval;
}    

function eqfn(v1, v2)
{
    return v1.equals(v2);
}    

function regular_compare(v1, v2)
{
    return v1 == v2;    
}    

function test(contractid, iid, d, string_val, val_compare_fn, str_compare_fn)
{
    var test1_result;    
    var test2_result;    
    var full_contractid = contractid_prefix+contractid+contractid_suffix;

    var clazz = Components.classes[full_contractid];
    if(!clazz) {
        println(full_contractid+ " is not a valid contractid");
        return false;
    }
    var v = clazz.createInstance(Components.interfaces[iid])
    v.data = d;

    if(!val_compare_fn)
        val_compare_fn = regular_compare;

    if(!str_compare_fn)
        str_compare_fn = regular_compare;

    test1_result = val_compare_fn(d,v.data);

    if(string_val) {
        if(string_val == same)
            test2_result = str_compare_fn(""+v, ""+d);
        else
            test2_result = str_compare_fn(""+v, string_val);
    }
    else
        test2_result = true;

    if(!test1_result)
        println("failed for... "+contractid+" with "+d+" returned "+v.data);

    if(!test2_result)
        println("failed toString for... "+contractid+" with "+d+" returned "+""+v);


    return test1_result && test2_result;
}    

var failureCount = 0;
var exception = null;

try {
    println("\n\n starting test... \n");
    for(i = 0; i < data.length; i++) {
        var r = data[i];
        if(!test(r[0], r[1], r[2], r[3], r[4], r[5])) {
            failureCount++ ;
        }
    }
} catch(e) {
    exception = e;
    failureCount++ ;
}    


if(exception)
    println("caught exception... "+exception);
println("\n"+(failureCount == 0 ? "all tests PASSED" : ""+failureCount+" errors"));
