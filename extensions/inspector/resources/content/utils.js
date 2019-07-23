

















































const kInspectorNSURI = "http://www.mozilla.org/inspector#";
const kXULNSURI = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const kHTMLNSURI = "http://www.w3.org/1999/xhtml";
const nsITransactionManager = Components.interfaces.nsITransactionManager;
var gEntityConverter;
const kCharTable = {
  '&': "&amp;",
  '<': "&lt;",
  '>': "&gt;",
  '"': "&quot;"
};

var InsUtil = {
  


  convertChromeURL: function(aURL)
  {
    var uri = XPCU.getService("@mozilla.org/network/io-service;1", "nsIIOService")
                  .newURI(aURL, null, null);
    var reg = XPCU.getService("@mozilla.org/chrome/chrome-registry;1", "nsIChromeRegistry");
    return reg.convertChromeURL(uri);
  },

  


  getDSProperty: function( aDS,  aId,  aPropName) 
  {
    var ruleRes = gRDF.GetResource(aId);
    var ds = XPCU.QI(aDS, "nsIRDFDataSource"); 
    var propRes = ds.GetTarget(ruleRes, gRDF.GetResource(kInspectorNSURI+aPropName), true);
    propRes = XPCU.QI(propRes, "nsIRDFLiteral");
    return propRes.Value;
  },
  
  


  persistAll: function(aId)
  {
    var el = document.getElementById(aId);
    if (el) {
      var attrs = el.getAttribute("persist").split(" ");
      for (var i = 0; i < attrs.length; ++i) {
        document.persist(aId, attrs[i]);
      }
    }
  },

  


  unicodeToEntity: function(text)
  {
    const entityVersion = Components.interfaces.nsIEntityConverter.entityW3C;

    function charTableLookup(letter) {
      return kCharTable[letter];
    }

    function convertEntity(letter) {
      try {
        return gEntityConverter.ConvertToEntity(letter, entityVersion);
      } catch (ex) {
        return letter;
      }
    }

    if (!gEntityConverter) {
      try {
        gEntityConverter =
          Components.classes["@mozilla.org/intl/entityconverter;1"]
                    .createInstance(Components.interfaces.nsIEntityConverter);
      } catch (ex) { }
    }

    
    text = text.replace(/[<>&"]/g, charTableLookup);

    
    text = text.replace(/[^\0-\u007f]/g, convertEntity);

    return text;
  }
};



function debug(aText)
{
  
  consoleDump(aText);
  
}


function consoleDump(aText)
{
  var csClass = Components.classes['@mozilla.org/consoleservice;1'];
  var cs = csClass.getService(Components.interfaces.nsIConsoleService);
  cs.logStringMessage(aText);
}

function dumpDOM(aNode, aIndent)
{
  if (!aIndent) aIndent = "";
  debug(aIndent + "<" + aNode.localName + "\n");
  var attrs = aNode.attributes;
  var attr;
  for (var a = 0; a < attrs.length; ++a) { 
    attr = attrs[a];
    debug(aIndent + "  " + attr.nodeName + "=\"" + attr.nodeValue + "\"\n");
  }
  debug(aIndent + ">\n");

  aIndent += "  ";
  
  for (var i = 0; i < aNode.childNodes.length; ++i)
    dumpDOM(aNode.childNodes[i], aIndent);
}



function txnQueryInterface(theUID, theResult)
{
  const nsITransaction = Components.interfaces.nsITransaction;
  const nsISupports    = Components.interfaces.nsISupports;
  if (theUID == nsITransaction || theUID == nsISupports) {
    return this;
  }
  return null;
}

function txnMerge()
{
  return false;
}

function txnRedoTransaction() {
  this.doTransaction();
}






function cmdEditCopy(aObjects) {
  
  this.txnType = "standard";

  
  this.QueryInterface = txnQueryInterface;
  this.merge = txnMerge;
  this.isTransient = true;

  this.objects = aObjects;
}

cmdEditCopy.prototype.doTransaction = 
function doTransaction() {
  if (this.objects.length == 1)
    viewer.pane.panelset.setClipboardData(this.objects[0],
                                          this.objects[0].flavor,
                                          this.objects.toString());
  else
    viewer.pane.panelset.setClipboardData(this.objects,
                                          this.objects[0].flavor + "s",
                                          this.objects.join(this.objects[0].delimiter));
}







function CSSDeclaration(aProperty, aValue, aImportant) {
  this.flavor = "inspector/css-declaration";
  this.delimiter = "\n";
  this.property = aProperty;
  this.value = aValue;
  this.important = aImportant == true;
}




CSSDeclaration.prototype.toString = function toString() {
  return this.property + ": " + this.value + (this.important ? " !important" : "") + ";";
}





function DOMAttribute(aNode)
{
  this.flavor = "inspector/dom-attribute";
  this.node = aNode.cloneNode(false);
  this.delimiter = " ";
}




DOMAttribute.prototype.toString = function toString()
{
  return this.node.nodeName + "=\"" + InsUtil.unicodeToEntity(this.node.nodeValue) + "\"";
};
