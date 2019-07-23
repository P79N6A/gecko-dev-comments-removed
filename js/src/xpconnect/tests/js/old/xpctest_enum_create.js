if (typeof document == "undefined") 
    dumpln = print;
else
    dumpln = function (str) {dump (str + "\n");}

function dumpObject (o, pfx, sep)
{
    var p;
    var s = "";

    sep = (typeof sep == "undefined") ? " = " : sep;
    pfx = (typeof pfx == "undefined") ? "" : pfx;
    

    for (p in o)
    {
        if (typeof (o[p]) != "function")
            s += pfx + p + sep + o[p] + "\n";
        else
            s += pfx + p + sep + "function\n";
    }

    return s;

}

function bruteForceEnumeration(ignore)
{
    var interfaceInfo = new Object();

loop: for (var c in Components.classes)
    {
        var cls = Components.classes[c];
        dumpln ("**");
        dumpln (" * ContractID: '" + cls.name + "'");
        dumpln (" * CLSID: " + cls.number);

        if(ignore) {
            for(var i = 0; i < ignore.length; i++) {
                if(0 == cls.name.indexOf(ignore[i])) {
                    dumpln (" * This one might cause a crash - SKIPPING");
                    continue loop;                
                }
            }
        }

        try
        {
            var ins = cls.createInstance();
            for (var i in Components.interfaces)
            {
                try
                {
                    qi = ins.QueryInterface (Components.interfaces[i]);
                    dumpln (" * Supports Interface: " + i);
                    if (typeof interfaceInfo[i] == "undefined")
                    {
                        interfaceInfo[i] = dumpObject (qi, " ** ");
                    }
                    dumpln (interfaceInfo[i]);
                }
                catch (e)
                {
                    
                }
            }
        }
        catch (e)
        {
            dumpln (" * createInstance FAILED:");
            dumpln (dumpObject (e));
        }
        
        dumpln ("");
        
    }

    dumpln ("**");
    dumpln (" * Interface Information :");
    dumpln ("");
    
    var bulk = dumpObject (interfaceInfo, (void 0), "::\n")
    var lines = bulk.split("\n");
    for(var i = 0; i < lines.length; i++)
        dumpln (lines[i]);
    
}


var totalClasses = 0,
        nonameClasses = 0,
        strangeClasses = 0,
        totalIfaces = 0,
        strangeIfaces = 0;

dumpln ("**> Enumerating Classes");
for (var c in Components.classes)
{
    totalClasses++;
    if (Components.classes[c].name == "")
    {
        dumpln ("CLSID " + c + " has no contractID.");
        nonameClasses++;
    }
    else
        if (c.search(/^component:\/\//) == -1)
        {
            dumpln ("Strange contractID '" + c + "'");
            strangeClasses++;
        }
}

dumpln ("**> Enumerating Interfaces");
for (var i in Components.interfaces)
{
    if (i != "QueryInterface")
    {
        totalIfaces++;
    
        if (i.search(/^nsI/) == -1)
        {
            dumpln ("Strange interface name '" + i + "'");
            strangeIfaces++;
        }
    }
}

dumpln ("** Enumerated " + totalClasses + " classes");
dumpln (" * " + nonameClasses + " without contractIDs");
dumpln (" * " + strangeClasses + " with strange names");
dumpln ("");
dumpln ("** Enumerated " + totalIfaces + " interfaces");
dumpln (" * " + strangeIfaces + " with strange names");

var contractIDsTo_NOT_Create = [












   "@mozilla.org/image/decoder;1?type=image/",  

    "@mozilla.org/wallet;1", 

    "@mozilla.org/messengercompose/compose;1",              
    "@mozilla.org/messenger;1",                             
    "@mozilla.org/rdf/datasource;1?name=msgaccountmanager", 
    "@mozilla.org/rdf/datasource;1?name=mailnewsfolders",   
    "@mozilla.org/rdf/datasource;1?name=msgnotifications",  
    "@mozilla.org/rdf/datasource;1?name=mailnewsmessages",  



    "@mozilla.org/rdf/xul-key-listener;1",   
    "@mozilla.org/rdf/xul-popup-listener;1", 
    "@mozilla.org/rdf/xul-focus-tracker;1",  


    "@mozilla.org/rdf/datasource;1?name=files", 
    "@mozilla.org/rdf/datasource;1?name=find", 

    "@mozilla.org/rdf/datasource;1?name=msgaccounts", 

    "@mozilla.org/rdf/xul-sort-service;1",  

    "componment://netscape/intl/charsetconvertermanager", 

    "@mozilla.org/rdf/datasource;1?name=mail-messageview", 


    "@mozilla.org/rdf/resource-factory;1",  


   "@mozilla.org/messenger/maildb;1", 

    "@mozilla.org/messenger/identity;1",  
    "@mozilla.org/messenger/server;1?type=", 

    "@mozilla.org/messenger/account;1",   

];


dumpln ("-------------------------------------------------------");
dumpln (" Now let's create every component we can find...");

if(contractIDsTo_NOT_Create.length) {
    dumpln ("...except the following 'cuz they've been know to cause CRASHES!...")
    for(var i = 0; i < contractIDsTo_NOT_Create.length; i++)
        dumpln ("  "+contractIDsTo_NOT_Create[i]);
    dumpln ();
}
bruteForceEnumeration(contractIDsTo_NOT_Create);
