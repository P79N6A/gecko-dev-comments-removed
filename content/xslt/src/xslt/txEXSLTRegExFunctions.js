






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const EXSLT_REGEXP_DESC = "EXSLT RegExp extension functions"

const CATMAN_CONTRACTID = "@mozilla.org/categorymanager;1";
const NODESET_CONTRACTID = "@mozilla.org/transformiix-nodeset;1";

const Ci = Components.interfaces;

function txEXSLTRegExFunctions()
{
}

txEXSLTRegExFunctions.prototype = {
    classID: Components.ID("{18a03189-067b-4978-b4f1-bafe35292ed6}"),

    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) ||
            iid.equals(Ci.txIEXSLTRegExFunctions))
            return this;

        if (iid.equals(Ci.nsIClassInfo))
            return txEXSLTRegExModule.factory

        throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    match: function(context, str, regex, flags) {
        var nodeset = Components.classes[NODESET_CONTRACTID]
                                .createInstance(Ci.txINodeSet);

        var re = new RegExp(regex, flags);
        var matches = str.match(re);
        if (matches != null && matches.length > 0) {
            var contextNode = context.contextNode;
            var doc = contextNode.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE ?
                      contextNode : contextNode.ownerDocument;
            var docFrag = doc.createDocumentFragment();

            for (var i = 0; i < matches.length; ++i) {
                var match = matches[i];
                var elem = doc.createElementNS(null, "match");
                var text = doc.createTextNode(match ? match : '');
                elem.appendChild(text);
                docFrag.appendChild(elem);
                nodeset.add(elem);
            }
        }

        return nodeset;
    },

    replace: function(str, regex, flags, replace) {
        var re = new RegExp(regex, flags);

        return str.replace(re, replace);
    },

    test: function(str, regex, flags) {
        var re = new RegExp(regex, flags);

        return re.test(str);
    }
}

var SingletonInstance = null;

function NSGetFactory(cid)
{
    if (!cid.equals(EXSLT_REGEXP_CID))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;


    return kFactory;
}

const kFactory = {
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) ||
            iid.equals(Ci.nsIFactory) ||
            iid.equals(Ci.nsIClassInfo))
            return this;

        throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    createInstance: function(outer, iid) {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;

        if (SingletonInstance == null)
            SingletonInstance = new txEXSLTRegExFunctions();

        return SingletonInstance.QueryInterface(iid);
    },

    getInterfaces: function(countRef) {
        var interfaces = [
            Ci.txIEXSLTRegExFunctions
        ];
        countRef.value = interfaces.length;

        return interfaces;
    },

    getHelperForLanguage: function(language) {
        return null;
    },

    contractID: EXSLT_REGEXP_CONTRACTID,
    classDescription: EXSLT_REGEXP_DESC,
    classID: EXSLT_REGEXP_CID,
    implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
    flags: Ci.nsIClassInfo.SINGLETON
};
