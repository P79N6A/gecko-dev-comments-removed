









































function println(s)
{
    if(this.document === undefined)
        print(s);
    else
        dump(s+"\n");
}


var nsID = Components.ID;



var NS_ISUPPORTS_IID = new nsID("{00000000-0000-0000-c000-000000000046}");
var NS_ECHO          = new nsID("{ed132c20-eed1-11d2-baa4-00805f8a5dd7}");
var NS_ECHO_UPPER    = new nsID("{ED132C20-EED1-11D2-BAA4-00805F8A5DD7}");
var NS_BOGUS_IID     = new nsID("{15898420-4d11-11d3-9893-006008962422}");


data = [
 
 [NS_ISUPPORTS_IID                   , "{00000000-0000-0000-c000-000000000046}"],
 
 [NS_ISUPPORTS_IID.name              , ""],
 
 [NS_ISUPPORTS_IID.number            , "{00000000-0000-0000-c000-000000000046}"],
 
 [NS_BOGUS_IID                       , "{15898420-4d11-11d3-9893-006008962422}"],

 
 [Components.interfaces.nsISupports  , "nsISupports"],
 [Components.classes["@mozilla.org/js/xpc/test/Echo;1"]
                                     , "@mozilla.org/js/xpc/test/Echo;1"],
 [Components.classes["@mozilla.org/js/xpc/test/Echo;1"].number   
                                     , "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],

 
 [Components.interfaces.bogus        , "undefined"],

 
 [Components.interfaces["{00000000-0000-0000-c000-000000000046}"] , "undefined"],
 [Components.classes["@mozilla.org/js/xpc/test/Echo;1"]["{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"], "undefined"],

 
 [NS_ECHO_UPPER                      , "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],
 [NS_ECHO                            , "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],
 [NS_ECHO_UPPER.number               , "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],
 [NS_ECHO.number                     , "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],

 
 [Components.classesByID.nsEcho      , "undefined"],
 [Components.classesByID["{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],       "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],
 [Components.classesByID["{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"].number, "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],

 
 [Components.classesByID["{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"], "{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"],

 
 [Components.classesByID["{35fb7000-4d23-11d3-9893-006008962422}"], "undefined"],

 
 [Components.classesByID["{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"].name,  ""],
 
 [Components.classesByID["{ed132c20-eed1-11d2-baa4-00805f8a5dd7}"].equals(
                    Components.classes["@mozilla.org/js/xpc/test/Echo;1"]), true],

 
 [Components.classes["@mozilla.org/js/xpc/test/Echo;1"].equals(
                    Components.classes["@mozilla.org/js/xpc/test/Echo;1"]), true],

 

]



println("\nJavaScript nsID tests...\n");
var failureCount = 0;
for(var i = 0; i < data.length; i++) {
    var item = data[i];
    if(""+item[0] != ""+item[1]) {
        println("failed for item "+i+" expected \""+item[1]+"\" got \""+item[0]+"\"");
        failureCount ++;
    }
}



println("");
if(failureCount)
    println(failureCount+" FAILURES total");
else
    println("all PASSED");
