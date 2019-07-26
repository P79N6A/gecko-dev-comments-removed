



"use strict";

this.EXPORTED_SYMBOLS = ["XPathGenerator"];

this.XPathGenerator = {
  
  namespaceURIs:     {
    "xhtml": "http://www.w3.org/1999/xhtml",
    "xul": "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
  },
  namespacePrefixes: {
    "http://www.w3.org/1999/xhtml": "xhtml",
    "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul": "xul"
  },

  


  generate: function sss_xph_generate(aNode) {
    
    if (!aNode.parentNode)
      return "";

    
    let nNamespaceURI = aNode.namespaceURI;
    let nLocalName = aNode.localName;

    let prefix = this.namespacePrefixes[nNamespaceURI] || null;
    let tag = (prefix ? prefix + ":" : "") + this.escapeName(nLocalName);

    
    if (aNode.id)
      return "//" + tag + "[@id=" + this.quoteArgument(aNode.id) + "]";

    
    
    let count = 0;
    let nName = aNode.name || null;
    for (let n = aNode; (n = n.previousSibling); )
      if (n.localName == nLocalName && n.namespaceURI == nNamespaceURI &&
          (!nName || n.name == nName))
        count++;

    
    return this.generate(aNode.parentNode) + "/" + tag +
           (nName ? "[@name=" + this.quoteArgument(nName) + "]" : "") +
           (count ? "[" + (count + 1) + "]" : "");
  },

  


  resolve: function sss_xph_resolve(aDocument, aQuery) {
    let xptype = Components.interfaces.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE;
    return aDocument.evaluate(aQuery, aDocument, this.resolveNS, xptype, null).singleNodeValue;
  },

  


  resolveNS: function sss_xph_resolveNS(aPrefix) {
    return XPathGenerator.namespaceURIs[aPrefix] || null;
  },

  


  escapeName: function sss_xph_escapeName(aName) {
    
    
    return /^\w+$/.test(aName) ? aName :
           "*[local-name()=" + this.quoteArgument(aName) + "]";
  },

  


  quoteArgument: function sss_xph_quoteArgument(aArg) {
    return !/'/.test(aArg) ? "'" + aArg + "'" :
           !/"/.test(aArg) ? '"' + aArg + '"' :
           "concat('" + aArg.replace(/'+/g, "',\"$&\",'") + "')";
  },

  


  get restorableFormNodes() {
    
    
    let ignoreTypes = ["password", "hidden", "button", "image", "submit", "reset"];
    
    let toLowerCase = '"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz"';
    let ignore = "not(translate(@type, " + toLowerCase + ")='" +
      ignoreTypes.join("' or translate(@type, " + toLowerCase + ")='") + "')";
    let formNodesXPath = "//textarea|//select|//xhtml:textarea|//xhtml:select|" +
      "//input[" + ignore + "]|//xhtml:input[" + ignore + "]";

    
    formNodesXPath += '|/xul:window[@id="config"]//xul:textbox[@id="textbox"]';

    delete this.restorableFormNodes;
    return (this.restorableFormNodes = formNodesXPath);
  }
};
