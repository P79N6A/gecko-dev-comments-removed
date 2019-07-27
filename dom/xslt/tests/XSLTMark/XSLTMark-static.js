




const enablePrivilege = netscape.security.PrivilegeManager.enablePrivilege;
const IOSERVICE_CTRID = "@mozilla.org/network/io-service;1";
const nsIIOService    = Components.interfaces.nsIIOService;
const SIS_CTRID       = "@mozilla.org/scriptableinputstream;1";
const nsISIS          = Components.interfaces.nsIScriptableInputStream;
const nsIFilePicker   = Components.interfaces.nsIFilePicker;
const STDURL_CTRID    = "@mozilla.org/network/standard-url;1";
const nsIURI          = Components.interfaces.nsIURI;

var gStop = false;

function loadFile(aUriSpec)
{
    enablePrivilege('UniversalXPConnect');
    var serv = Components.classes[IOSERVICE_CTRID].
        getService(nsIIOService);
    if (!serv) {
        throw Components.results.ERR_FAILURE;
    }
    var chan = serv.newChannel2(aUriSpec,
                                null,
                                null,
                                null,      
                                Services.scriptSecurityManager.getSystemPrincipal(),
                                null,      
                                Ci.nsILoadInfo.SEC_NORMAL,
                                Ci.nsIContentPolicy.TYPE_OTHER);
    var instream = 
        Components.classes[SIS_CTRID].createInstance(nsISIS);
    instream.init(chan.open());

    return instream.read(instream.available());
}

function dump20(aVal)
{
    const pads = '                    ';
    if (typeof(aVal)=='string')
        out = aVal;
    else if (typeof(aVal)=='number')
        out = Number(aVal).toFixed(2);
    else
        out = new String(aVal);
    dump(pads.substring(0, 20 - out.length));
    dump(out);
}
