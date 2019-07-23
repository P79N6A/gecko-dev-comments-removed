








































function nsNativeEcho()
{
    var clazz = Components.classes["@mozilla.org/js/xpc/test/Echo;1"];
    var iface = Components.interfaces.nsIEcho;
    return new clazz(iface);
}    

var e = nsNativeEcho();

e.printArgTypes();
e.printArgTypes(1);
e.printArgTypes(1,2);
e.printArgTypes(null, new Object(), 1.1, undefined);

try {
    e.throwArg(1);    
} catch(e) {
    print(e);
}

try {
    e.throwArg("this is a string to throw");    
} catch(e) {
    print(e);
}


