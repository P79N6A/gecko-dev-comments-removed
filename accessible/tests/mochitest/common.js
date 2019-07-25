


const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;

const nsIAccessibleEvent = Components.interfaces.nsIAccessibleEvent;
const nsIAccessibleStateChangeEvent =
  Components.interfaces.nsIAccessibleStateChangeEvent;
const nsIAccessibleCaretMoveEvent =
  Components.interfaces.nsIAccessibleCaretMoveEvent;
const nsIAccessibleTextChangeEvent =
  Components.interfaces.nsIAccessibleTextChangeEvent;

const nsIAccessibleStates = Components.interfaces.nsIAccessibleStates;
const nsIAccessibleRole = Components.interfaces.nsIAccessibleRole;
const nsIAccessibleTypes = Components.interfaces.nsIAccessibleTypes;

const nsIAccessibleRelation = Components.interfaces.nsIAccessibleRelation;

const nsIAccessNode = Components.interfaces.nsIAccessNode;
const nsIAccessible = Components.interfaces.nsIAccessible;

const nsIAccessibleCoordinateType =
      Components.interfaces.nsIAccessibleCoordinateType;

const nsIAccessibleDocument = Components.interfaces.nsIAccessibleDocument;
const nsIAccessibleApplication = Components.interfaces.nsIAccessibleApplication;

const nsIAccessibleText = Components.interfaces.nsIAccessibleText;
const nsIAccessibleEditableText = Components.interfaces.nsIAccessibleEditableText;

const nsIAccessibleHyperLink = Components.interfaces.nsIAccessibleHyperLink;
const nsIAccessibleHyperText = Components.interfaces.nsIAccessibleHyperText;

const nsIAccessibleImage = Components.interfaces.nsIAccessibleImage;
const nsIAccessibleSelectable = Components.interfaces.nsIAccessibleSelectable;
const nsIAccessibleTable = Components.interfaces.nsIAccessibleTable;
const nsIAccessibleTableCell = Components.interfaces.nsIAccessibleTableCell;
const nsIAccessibleValue = Components.interfaces.nsIAccessibleValue;

const nsIObserverService = Components.interfaces.nsIObserverService;

const nsIDOMDocument = Components.interfaces.nsIDOMDocument;
const nsIDOMEvent = Components.interfaces.nsIDOMEvent;
const nsIDOMHTMLDocument = Components.interfaces.nsIDOMHTMLDocument;
const nsIDOMNode = Components.interfaces.nsIDOMNode;
const nsIDOMNSHTMLElement = Components.interfaces.nsIDOMNSHTMLElement;
const nsIDOMWindow = Components.interfaces.nsIDOMWindow;
const nsIDOMXULElement = Components.interfaces.nsIDOMXULElement;

const nsIPropertyElement = Components.interfaces.nsIPropertyElement;



const MAC = (navigator.platform.indexOf("Mac") != -1)? true : false;
const LINUX = (navigator.platform.indexOf("Linux") != -1)? true : false;
const SOLARIS = (navigator.platform.indexOf("SunOS") != -1)? true : false;
const WIN = (navigator.platform.indexOf("Win") != -1)? true : false;




const STATE_BUSY = nsIAccessibleStates.STATE_BUSY;

const kEmbedChar = String.fromCharCode(0xfffc);

const kDiscBulletText = String.fromCharCode(0x2022) + " ";
const kCircleBulletText = String.fromCharCode(0x25e6) + " ";
const kSquareBulletText = String.fromCharCode(0x25aa) + " ";




var gAccRetrieval = null;








function addA11yLoadEvent(aFunc)
{
  function waitForDocLoad()
  {
    window.setTimeout(
      function()
      {
        var accDoc = getAccessible(document);
        var state = {};
        accDoc.getState(state, {});
        if (state.value & STATE_BUSY)
          return waitForDocLoad();

        window.setTimeout(aFunc, 0);
      },
      0
    );
  }

  SimpleTest.waitForFocus(waitForDocLoad);
}







function getNode(aAccOrNodeOrID)
{
  if (!aAccOrNodeOrID)
    return null;

  if (aAccOrNodeOrID instanceof nsIDOMNode)
    return aAccOrNodeOrID;

  if (aAccOrNodeOrID instanceof nsIAccessible) {
    aAccOrNodeOrID.QueryInterface(nsIAccessNode);
    return aAccOrNodeOrID.DOMNode;
  }

  node = document.getElementById(aAccOrNodeOrID);
  if (!node) {
    ok(false, "Can't get DOM element for " + aAccOrNodeOrID);
    return null;
  }

  return node;
}




const DONOTFAIL_IF_NO_ACC = 1;





const DONOTFAIL_IF_NO_INTERFACE = 2;














function getAccessible(aAccOrElmOrID, aInterfaces, aElmObj, aDoNotFailIf)
{
  if (!aAccOrElmOrID)
    return null;

  var elm = null;

  if (aAccOrElmOrID instanceof nsIAccessible) {
    aAccOrElmOrID.QueryInterface(nsIAccessNode);
    elm = aAccOrElmOrID.DOMNode;

  } else if (aAccOrElmOrID instanceof nsIDOMNode) {
    elm = aAccOrElmOrID;

  } else {
    elm = document.getElementById(aAccOrElmOrID);
    if (!elm) {
      ok(false, "Can't get DOM element for " + aAccOrElmOrID);
      return null;
    }
  }

  if (aElmObj && (typeof aElmObj == "object"))
    aElmObj.value = elm;

  var acc = (aAccOrElmOrID instanceof nsIAccessible) ? aAccOrElmOrID : null;
  if (!acc) {
    try {
      acc = gAccRetrieval.getAccessibleFor(elm);
    } catch (e) {
    }

    if (!acc) {
      if (!(aDoNotFailIf & DONOTFAIL_IF_NO_ACC))
        ok(false, "Can't get accessible for " + aAccOrElmOrID);

      return null;
    }
  }

  acc.QueryInterface(nsIAccessNode);

  if (!aInterfaces)
    return acc;

  if (aInterfaces instanceof Array) {
    for (var index = 0; index < aInterfaces.length; index++) {
      try {
        acc.QueryInterface(aInterfaces[index]);
      } catch (e) {
        if (!(aDoNotFailIf & DONOTFAIL_IF_NO_INTERFACE))
          ok(false, "Can't query " + aInterfaces[index] + " for " + aAccOrElmOrID);

        return null;
      }
    }
    return acc;
  }
  
  try {
    acc.QueryInterface(aInterfaces);
  } catch (e) {
    ok(false, "Can't query " + aInterfaces + " for " + aAccOrElmOrID);
    return null;
  }
  
  return acc;
}





function isAccessible(aAccOrElmOrID, aInterfaces)
{
  return getAccessible(aAccOrElmOrID, aInterfaces, null,
                       DONOTFAIL_IF_NO_ACC | DONOTFAIL_IF_NO_INTERFACE) ?
    true : false;
}




function getContainerAccessible(aAccOrElmOrID)
{
  var node = getNode(aAccOrElmOrID);
  if (!node)
    return null;

  while ((node = node.parentNode) && !isAccessible(node));
  return node ? getAccessible(node) : null;
}




function getRootAccessible(aAccOrElmOrID)
{
  var acc = getAccessible(aAccOrElmOrID ? aAccOrElmOrID : document,
                          [nsIAccessNode]);
  return acc ? acc.rootDocument.QueryInterface(nsIAccessible) : null;
}




function getTabDocAccessible(aAccOrElmOrID)
{
  var acc = getAccessible(aAccOrElmOrID ? aAccOrElmOrID : document,
                          [nsIAccessNode]);

  var docAcc = acc.document.QueryInterface(nsIAccessible);
  var containerDocAcc = docAcc.parent.QueryInterface(nsIAccessNode).document;

  
  if (acc.rootDocument == containerDocAcc)
    return docAcc;

  
  return containerDocAcc.QueryInterface(nsIAccessible);
}




function getApplicationAccessible()
{
  return gAccRetrieval.getApplicationAccessible().
    QueryInterface(nsIAccessibleApplication);
}





function ensureAccessibleTree(aAccOrElmOrID)
{
  var acc = getAccessible(aAccOrElmOrID);
  if (!acc)
    return;

  var child = acc.firstChild;
  while (child) {
    ensureAccessibleTree(child);
    try {
      child = child.nextSibling;
    } catch (e) {
      child = null;
    }
  }
}













function testAccessibleTree(aAccOrElmOrID, aAccTree)
{
  var acc = getAccessible(aAccOrElmOrID);
  if (!acc)
    return;

  var accTree = aAccTree;

  
  var key = Object.keys(accTree)[0];
  var roleName = "ROLE_" + key;
  if (roleName in nsIAccessibleRole) {
    accTree = {
      role: nsIAccessibleRole[roleName],
      children: accTree[key]
    };
  }

  
  for (var prop in accTree) {
    var msg = "Wrong value of property '" + prop + "' for " + prettyName(acc) + ".";
    if (prop == "role") {
      is(roleToString(acc[prop]), roleToString(accTree[prop]), msg);

    } else if (prop == "states") {
      var statesObj = accTree[prop];
      testStates(acc, statesObj.states, statesObj.extraStates,
                 statesObj.absentStates, statesObj.absentExtraStates);

    } else if (prop != "children") {
      is(acc[prop], accTree[prop], msg);
    }
  }

  
  if ("children" in accTree && accTree["children"] instanceof Array) {
    var children = acc.children;
    is(children.length, accTree.children.length,
       "Different amount of expected children of " + prettyName(acc) + ".");

    if (accTree.children.length == children.length) {
      var childCount = children.length;

      
      var expectedFirstChild = childCount > 0 ?
        children.queryElementAt(0, nsIAccessible) : null;
      var firstChild = null;
      try { firstChild = acc.firstChild; } catch (e) {}
      is(firstChild, expectedFirstChild,
         "Wrong first child of " + prettyName(acc));

      
      var expectedLastChild = childCount > 0 ?
        children.queryElementAt(childCount - 1, nsIAccessible) : null;
      var lastChild = null;
      try { lastChild = acc.lastChild; } catch (e) {}
      is(lastChild, expectedLastChild,
         "Wrong last child of " + prettyName(acc));

      for (var i = 0; i < children.length; i++) {
        var child = children.queryElementAt(i, nsIAccessible);

        
        var parent = null;
        try { parent = child.parent; } catch (e) {}
        is(parent, acc, "Wrong parent of " + prettyName(child));

        
        var indexInParent = -1;
        try { indexInParent = child.indexInParent; } catch(e) {}
        is(indexInParent, i,
           "Wrong index in parent of " + prettyName(child));

        
        var expectedNextSibling = (i < childCount - 1) ?
          children.queryElementAt(i + 1, nsIAccessible) : null;
        var nextSibling = null;
        try { nextSibling = child.nextSibling; } catch (e) {}
        is(nextSibling, expectedNextSibling,
           "Wrong next sibling of " + prettyName(child));

        
        var expectedPrevSibling = (i > 0) ?
          children.queryElementAt(i - 1, nsIAccessible) : null;
        var prevSibling = null;
        try { prevSibling = child.previousSibling; } catch (e) {}
        is(prevSibling, expectedPrevSibling,
           "Wrong previous sibling of " + prettyName(child));

        
        testAccessibleTree(child, accTree.children[i]);
      }
    }
  }
}




function isAccessibleInCache(aNodeOrId)
{
  var node = getNode(aNodeOrId);
  return gAccRetrieval.getAccessibleFromCache(node) ? true : false;
}







function testDefunctAccessible(aAcc, aNodeOrId)
{
  if (aNodeOrId)
    ok(!isAccessible(aNodeOrId),
       "Accessible for " + aNodeOrId + " wasn't properly shut down!");

  var msg = " doesn't fail for shut down accessible " + prettyName(aNodeOrId) + "!";

  
  var success = false;
  try {
    aAcc.firstChild;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE)
  }
  ok(success, "firstChild" + msg);

  
  success = false;
  try {
    aAcc.lastChild;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE)
  }
  ok(success, "lastChild" + msg);

  
  success = false;
  try {
    aAcc.childCount;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE)
  }
  ok(success, "childCount" + msg);

  
  success = false;
  try {
    aAcc.children;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE)
  }
  ok(success, "children" + msg);

  
  success = false;
  try {
    aAcc.nextSibling;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE);
  }
  ok(success, "nextSibling" + msg);

  
  success = false;
  try {
    aAcc.previousSibling;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE);
  }
  ok(success, "previousSibling" + msg);

  
  success = false;
  try {
    aAcc.parent;
  } catch (e) {
    success = (e.result == Components.results.NS_ERROR_FAILURE);
  }
  ok(success, "parent" + msg);
}





function roleToString(aRole)
{
  return gAccRetrieval.getStringRole(aRole);
}




function statesToString(aStates, aExtraStates)
{
  var list = gAccRetrieval.getStringStates(aStates, aExtraStates);

  var str = "";
  for (var index = 0; index < list.length - 1; index++)
    str += list.item(index) + ", ";

  if (list.length != 0)
    str += list.item(index)

  return str;
}




function eventTypeToString(aEventType)
{
  return gAccRetrieval.getStringEventType(aEventType);
}




function relationTypeToString(aRelationType)
{
  return gAccRetrieval.getStringRelationType(aRelationType);
}




function getTextFromClipboard()
{
  var clip = Components.classes["@mozilla.org/widget/clipboard;1"].
    getService(Components.interfaces.nsIClipboard);
  if (!clip)
    return;

  var trans = Components.classes["@mozilla.org/widget/transferable;1"].
    createInstance(Components.interfaces.nsITransferable);
  if (!trans)
    return;

  trans.addDataFlavor("text/unicode");
  clip.getData(trans, clip.kGlobalClipboard);

  var str = new Object();
  var strLength = new Object();
  trans.getTransferData("text/unicode", str, strLength);

  if (str)
    str = str.value.QueryInterface(Components.interfaces.nsISupportsString);
  if (str)
    return str.data.substring(0, strLength.value / 2);

  return "";
}




function prettyName(aIdentifier)
{
  if (aIdentifier instanceof nsIAccessible) {
    var acc = getAccessible(aIdentifier, [nsIAccessNode]);
    var msg = "[" + getNodePrettyName(acc.DOMNode);
    try {
      msg += ", role: " + roleToString(acc.role);
      if (acc.name)
        msg += ", name: '" + acc.name + "'";
    } catch (e) {
      msg += "defunct";
    }

    if (acc) {
      var exp = /native\s*@\s*(0x[a-f0-9]+)/g;
      var match = exp.exec(acc.valueOf());
      if (match)
        msg += ", address: " + match[1];
      else
        msg += ", address: " + acc.valueOf();
    }
    msg += "]";

    return msg;
  }

  if (aIdentifier instanceof nsIDOMNode)
    return getNodePrettyName(aIdentifier);

  return " '" + aIdentifier + "' ";
}








function initialize()
{
  gAccRetrieval = Components.classes["@mozilla.org/accessibleRetrieval;1"].
    getService(nsIAccessibleRetrieval);
}

addLoadEvent(initialize);

function getNodePrettyName(aNode)
{
  try {
    if (aNode.nodeType == nsIDOMNode.DOCUMENT_NODE)
      return " 'document node' ";

    var name = " '" + aNode.localName;
    if (aNode.nodeType == nsIDOMNode.ELEMENT_NODE && aNode.hasAttribute("id"))
      name += "@id='" + aNode.getAttribute("id") + "'";

    name += " node' "
    return name;
  } catch (e) {
    return "' no node info '";
  }
}
