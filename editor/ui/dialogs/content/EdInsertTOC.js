






































var tocHeadersArray = new Array(6);


var currentHeaderLevel = 0;


var readonly = false;


var orderedList = true;


const kMozToc                  = "mozToc";
const kMozTocLength            = 6;
const kMozTocIdPrefix          = "mozTocId";
const kMozTocIdPrefixLength    = 8;
const kMozTocClassPrefix       = "mozToc";
const kMozTocClassPrefixLength = 6;


function Startup()
{
  
  if (!GetCurrentEditor()) {
    window.close();
    return;
  }

  var i, j;
  
  for (i = 0; i < 6; ++i)
    tocHeadersArray[i] = [ "", "" ];

  
  for (i = 1; i < 7; ++i) {
    var menulist = document.getElementById("header" + i + "Menulist");
    var menuitem = document.getElementById("header" + i + "none");
    var textbox  = document.getElementById("header" + i + "Class");
    menulist.selectedItem = menuitem;
    textbox.setAttribute("disabled", "true");
  }

  var theDocument = GetCurrentEditor().document;

  
  var toc = theDocument.getElementById(kMozToc);

  
  var headers = "h1 1 h2 2 h3 3 h4 4 h5 5 h6 6";

  var orderedListCheckbox = document.getElementById("orderedListCheckbox");
  orderedListCheckbox.checked = true;

  if (toc) {
    

    if (toc.getAttribute("class") == "readonly") {
      
      var checkbox = document.getElementById("readOnlyCheckbox");
      checkbox.checked = true;
      readonly = true;
    }

    
    orderedList = (toc.nodeName.toLowerCase() == "ol");
    orderedListCheckbox.checked = orderedList;

    var nodeList = toc.childNodes;
    
    
    for (i = 0; i< nodeList.length; ++i) {
      if (nodeList.item(i).nodeType == Node.COMMENT_NODE &&
          nodeList.item(i).data.substr(0, kMozTocLength) == kMozToc) {
        
        headers = nodeList.item(i).data.substr(kMozTocLength + 1,
                                    nodeList.item(i).length - kMozTocLength - 1);
        break;
      }
    }
  }

  
  var headersArray = headers.split(" ");

  for (i = 0; i < headersArray.length; i += 2) {
    var tag = headersArray[i], className = "";
    var index = headersArray[i + 1];
    menulist = document.getElementById("header" + index + "Menulist");
    if (menulist) {
      var sep = tag.indexOf(".");
      if (sep != -1) {
        
        
        var tmp   = tag.substr(0, sep);
        className = tag.substr(sep + 1, tag.length - sep - 1);
        tag = tmp;
      }

      
      menuitem = document.getElementById("header" + index +
                                         tag.toUpperCase());
      textbox  = document.getElementById("header" + index + "Class");
      menulist.selectedItem = menuitem;
      if (tag != "") {
        textbox.removeAttribute("disabled");
      }
      if (className != "") {
        textbox.value = className;
      }
      tocHeadersArray[index - 1] = [ tag, className ];
    }
  }
}


function BuildTOC(update)
{
  
  
  
  function controlClass(node, index)
  {
    currentHeaderLevel = index + 1;
    if (tocHeadersArray[index][1] == "") {
      
      return NodeFilter.FILTER_ACCEPT;
    }
    if (node.getAttribute("class")) {
      
      
      var classArray = node.getAttribute("class").split(" ");
      for (var j = 0; j < classArray.length; j++) {
        if (classArray[j] == tocHeadersArray[index][1]) {
          
          return NodeFilter.FILTER_ACCEPT;
        }
      }
    }
    return NodeFilter.FILTER_SKIP;
  }

  
  
  
  function acceptNode(node)
  {
    switch (node.nodeName.toLowerCase())
    {
      case tocHeadersArray[0][0]:
        return controlClass(node, 0);
        break;
      case tocHeadersArray[1][0]:
        return controlClass(node, 1);
        break;
      case tocHeadersArray[2][0]:
        return controlClass(node, 2);
        break;
      case tocHeadersArray[3][0]:
        return controlClass(node, 3);
        break;
      case tocHeadersArray[4][0]:
        return controlClass(node, 4);
        break;
      case tocHeadersArray[5][0]:
        return controlClass(node, 5);
        break;
      default:
        return NodeFilter.FILTER_SKIP;
        break;
    }
    return NodeFilter.FILTER_SKIP;   
  }

  var editor = GetCurrentEditor();
  var theDocument = editor.document;
  
  var treeWalker = theDocument.createTreeWalker(theDocument.documentElement,
                                                NodeFilter.SHOW_ELEMENT,
                                                acceptNode,
                                                true);
  
  var tocArray = new Array();
  if (treeWalker) {
    var tocSourceNode = treeWalker.nextNode();
    while (tocSourceNode) {
      var headerIndex = currentHeaderLevel;

      
      var textTreeWalker = theDocument.createTreeWalker(tocSourceNode,
                                                        NodeFilter.SHOW_TEXT,
                                                        null,
                                                        true);
      var textNode = textTreeWalker.nextNode(), headerText = "";
      while (textNode) {
        headerText += textNode.data;
        textNode = textTreeWalker.nextNode();
      }

      var anchor = tocSourceNode.firstChild, id;
      
      if (anchor.nodeName.toLowerCase() == "a" &&
          anchor.hasAttribute("name") &&
          anchor.getAttribute("name").substr(0, kMozTocIdPrefixLength) == kMozTocIdPrefix) {
        
        id = anchor.getAttribute("name");
      }
      else {
        
        anchor = theDocument.createElement("a");
        tocSourceNode.insertBefore(anchor, tocSourceNode.firstChild);
        
        var c = 1000000 * Math.random();
        id = kMozTocIdPrefix + Math.round(c);
        anchor.setAttribute("name",  id);
        anchor.setAttribute("class", kMozTocClassPrefix +
                                     tocSourceNode.nodeName.toUpperCase());
      }
      
      tocArray.push(headerIndex, headerText, id);
      tocSourceNode = treeWalker.nextNode();
    }
  }

  
  headerIndex = 0;
  var item, toc;
  for (var i = 0; i < tocArray.length; i += 3) {
    if (!headerIndex) {
      
      ++headerIndex;
      toc = theDocument.getElementById(kMozToc);
      if (!toc || !update) {
        
        toc = GetCurrentEditor().createElementWithDefaults(orderedList ? "ol" : "ul");
        
        
        var pit = theDocument.createElement("li");
        toc.appendChild(pit);
        GetCurrentEditor().insertElementAtSelection(toc, true);
        
        toc.removeChild(pit);
        
        toc.setAttribute("id", kMozToc);
      }
      else {
        
        
        if (orderedList != (toc.nodeName.toLowerCase() == "ol")) {
          
          var newToc = GetCurrentEditor().createElementWithDefaults(orderedList ? "ol" : "ul");
          toc.parentNode.insertBefore(newToc, toc);
          
          toc.parentNode.removeChild(toc);
          toc = newToc;
          toc.setAttribute("id", kMozToc);
        }
        else {
          
          while (toc.hasChildNodes()) 
            toc.removeChild(toc.lastChild);
        }
      }
      var commentText = "mozToc ";
      for (var j = 0; j < 6; j++) {
        if (tocHeadersArray[j][0] != "") {
          commentText += tocHeadersArray[j][0];
          if (tocHeadersArray[j][1] != "") {
            commentText += "." + tocHeadersArray[j][1];
          }
          commentText += " " + (j + 1) + " ";
        }
      }
      
      commentText = TrimStringRight(commentText);

      
      
      var ct = theDocument.createComment(commentText);
      toc.appendChild(ct);

      
      
      if (readonly) {
        toc.setAttribute("class", "readonly");
      }
      else {
        toc.removeAttribute("class");
      }

      
      
      
      var tocList = toc;
      
      var tocItem = theDocument.createElement("li");
      
      var tocAnchor = theDocument.createElement("a");
      
      tocAnchor.setAttribute("href", "#" + tocArray[i + 2]);
      
      var tocEntry = theDocument.createTextNode(tocArray[i + 1]);
      
      tocAnchor.appendChild(tocEntry);
      tocItem.appendChild(tocAnchor);
      tocList.appendChild(tocItem);
      item = tocList;
    }
    else {
      if (tocArray[i] < headerIndex) {
        
        
        for (j = headerIndex - tocArray[i]; j > 0; --j) {
          if (item != toc) {
            item = item.parentNode.parentNode;
          }
        }
        tocItem = theDocument.createElement("li");
      }
      else if (tocArray[i] > headerIndex) {
        
        
        for (j = tocArray[i] - headerIndex; j > 0; --j) {
          tocList = theDocument.createElement(orderedList ? "ol" : "ul");
          item.lastChild.appendChild(tocList);
          tocItem = theDocument.createElement("li");
          tocList.appendChild(tocItem);
          item = tocList;
        }
      }
      else {
        tocItem = theDocument.createElement("li");
      }
      tocAnchor = theDocument.createElement("a");
      tocAnchor.setAttribute("href", "#" + tocArray[i + 2]);
      tocEntry = theDocument.createTextNode(tocArray[i + 1]);
      tocAnchor.appendChild(tocEntry);
      tocItem.appendChild(tocAnchor);
      item.appendChild(tocItem);
      headerIndex = tocArray[i];
    }
  }
  SaveWindowLocation();
  return true;
}

function selectHeader(elt, index)
{
  var tag = elt.value;
  tocHeadersArray[index - 1][0] = tag;
  var textbox = document.getElementById("header" + index + "Class");
  if (tag == "") {
    textbox.setAttribute("disabled", "true");
  }
  else {
    textbox.removeAttribute("disabled");
  }
}

function changeClass(elt, index)
{
  tocHeadersArray[index - 1][1] = elt.value;
}

function ToggleReadOnlyToc(elt)
{
  readonly = elt.checked;
}

function ToggleOrderedList(elt)
{
  orderedList = elt.checked;
}
