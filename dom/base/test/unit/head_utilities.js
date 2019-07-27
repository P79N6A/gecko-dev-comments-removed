




Components.utils.import("resource://testing-common/httpd.js");
Components.utils.import("resource://gre/modules/Services.jsm");

const nsIDocumentEncoder = Components.interfaces.nsIDocumentEncoder;
const replacementChar = Components.interfaces.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER;

function loadContentFile(aFile, aCharset) {
    
    if(aCharset == undefined)
        aCharset = 'UTF-8';

    var file = do_get_file(aFile);
    var ios = Components.classes['@mozilla.org/network/io-service;1']
            .getService(Components.interfaces.nsIIOService);
    var chann = ios.newChannelFromURI2(ios.newFileURI(file),
                                       null,      
                                       Services.scriptSecurityManager.getSystemPrincipal(),
                                       null,      
                                       Components.interfaces.nsILoadInfo.SEC_NORMAL,
                                       Components.interfaces.nsIContentPolicy.TYPE_OTHER);
    chann.contentCharset = aCharset;

    





    var inputStream = Components.classes["@mozilla.org/intl/converter-input-stream;1"]
                       .createInstance(Components.interfaces.nsIConverterInputStream);
    inputStream.init(chann.open(), aCharset, 1024, replacementChar);
    var str = {}, content = '';
    while (inputStream.readString(4096, str) != 0) {
        content += str.value;
    }
    return content;
}
