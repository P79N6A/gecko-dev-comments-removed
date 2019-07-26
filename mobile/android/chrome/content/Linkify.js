



const LINKIFY_TIMEOUT = 0;

function Linkifier() {
  this._linkifyTimer = null;
  this._phoneRegex = /(?:\s|^)[\+]?(\(?\d{1,8}\)?)?([- ]+\(?\d{1,8}\)?)+( ?(x|ext) ?\d{1,3})?(?:\s|$)/g;
}

Linkifier.prototype = {
  _buildAnchor : function(aDoc, aNumberText) {
    let anchorNode = aDoc.createElement("a");
    let cleanedText = "";
    for (let i = 0; i < aNumberText.length; i++) {
      let c = aNumberText.charAt(i);
      if ((c >= '0' && c <= '9') || c == '+')  
        cleanedText += c;
    }
    anchorNode.setAttribute("href", "tel:" + cleanedText);
    let nodeText = aDoc.createTextNode(aNumberText);
    anchorNode.appendChild(nodeText);
    return anchorNode;
  },

  _linkifyNodeNumbers : function(aNodeToProcess, aDoc) {
    let parent = aNodeToProcess.parentNode;
    let nodeText = aNodeToProcess.nodeValue;

    
    
    
    let anchorNode = null;
    let m = null;
    let startIndex = 0;
    let prevNode = null;
    while (m = this._phoneRegex.exec(nodeText)) {
      anchorNode = this._buildAnchor(aDoc, nodeText.substr(m.index, m[0].length));

      let textExistsBeforeNumber = (m.index > startIndex);
      let nodeToAdd = null;
      if (textExistsBeforeNumber)
        nodeToAdd = aDoc.createTextNode(nodeText.substr(startIndex, m.index - startIndex));
      else
        nodeToAdd = anchorNode;

      if (!prevNode) 
        parent.replaceChild(nodeToAdd, aNodeToProcess);
      else
        parent.insertBefore(nodeToAdd, prevNode.nextSibling); 

      if (textExistsBeforeNumber) 
        parent.insertBefore(anchorNode, nodeToAdd.nextSibling);

      
      prevNode = anchorNode; 
      startIndex = m.index + m[0].length;
    }

    
    if (startIndex > 0 && startIndex < nodeText.length) {
      let lastNode = aDoc.createTextNode(nodeText.substr(startIndex));
      parent.insertBefore(lastNode, prevNode.nextSibling);
      return lastNode;
    }
    return anchorNode;
  },

  linkifyNumbers: function(aDoc) {
    
    if (this._linkifyTimer) {
      clearTimeout(this._linkifyTimer);
      this._linkifyTimer = null;
    }

    let filterNode = function (node) {
      if (node.parentNode.tagName != 'A' &&
         node.parentNode.tagName != 'SCRIPT' &&
         node.parentNode.tagName != 'NOSCRIPT' &&
         node.parentNode.tagName != 'STYLE' &&
         node.parentNode.tagName != 'APPLET' &&
         node.parentNode.tagName != 'TEXTAREA')
        return NodeFilter.FILTER_ACCEPT;
      else
        return NodeFilter.FILTER_REJECT;
    }

    let nodeWalker = aDoc.createTreeWalker(aDoc.body, NodeFilter.SHOW_TEXT, filterNode, false);
    let parseNode = function() {
      let node = nodeWalker.nextNode();
      if (!node) {
        this._linkifyTimer = null;
        return;
      }
      let lastAddedNode = this._linkifyNodeNumbers(node, aDoc);
      
      if (lastAddedNode) {
        nodeWalker.currentNode = lastAddedNode;
        this._linkifyTimer = setTimeout(parseNode, LINKIFY_TIMEOUT);
      } else {
        this._linkifyTimer = setTimeout(parseNode, LINKIFY_TIMEOUT);
      }
    }.bind(this);

    this._linkifyTimer = setTimeout(parseNode, LINKIFY_TIMEOUT); 
  }
};
