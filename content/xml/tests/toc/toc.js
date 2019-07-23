




































function toggleDisplay(event)
{
  if (event.target.localName != "IMG")
    return;
  var img = event.target;
  var div = img.nextSibling.nextSibling; 

  
  
  if (div.style.display == "none") {
    div.style.display = "block";
    img.src = "minus.gif";
  }
  else {
    div.style.display = "none";
    img.src = "plus.gif";
  }
}





var searchTags = new Array("book", "chapter", "section");
var tocTags = new Array("level1", "level2", "level3");
function addToToc(root, tocFrame)
{
  var i;
  var newTocFrame = tocFrame;
  var newTocElement = null;
  var newTocLink = null;

  for (i=0; i < searchTags.length; i++) {
    if (root.tagName == searchTags[i]) {
      
      
      newTocElement = document.createElement(tocTags[i]);
      
      
      newTocLink = document.createElement("toclink");
      newTocLink.setAttributeNS("http://www.w3.org/1999/xlink","xlink:type", "simple");
      newTocLink.setAttributeNS("http://www.w3.org/1999/xlink","xlink:href", "#"+ root.getAttribute("id"));
      newTocLink.setAttributeNS("http://www.w3.org/1999/xlink","xlink:show", "replace");
      newTocElement.appendChild(newTocLink);

      
      if (i < searchTags.length-1) {
        var img = document.createElementNS("http://www.w3.org/1999/xhtml","img");
        img.src = "minus.gif";
        newTocElement.insertBefore(img,newTocLink);
 
        newTocFrame = document.createElementNS("http://www.w3.org/1999/xhtml","div");
        newTocElement.appendChild(newTocFrame);
      }
      else {
        newTocFrame = null;
      }

      tocFrame.appendChild(newTocElement);

      break;
    }
  }

  
  for (i=0; i < root.childNodes.length; i++) {
    var child = root.childNodes[i];
    if (child.nodeType == Node.ELEMENT_NODE) {
      if ((newTocLink != null) && (child.tagName == "title")) {
        var text = child.firstChild.cloneNode(true);
        newTocLink.appendChild(text);
      }
      else {
        addToToc(child, newTocFrame);
      }
    }
  }
}



function createToc()
{
  if (document.getElementsByTagName("toc").length == 0) {
    var toc = document.createElement("toc");
    var title = document.createElement("title");
    title.appendChild(document.createTextNode("Table of Contents"));
    toc.appendChild(title);
  
    
    addToToc(document.documentElement, toc);
    
    
    
    
    document.styleSheets[0].cssRules[0].style.marginLeft = "12em";
    document.documentElement.appendChild(toc);
    
    
    
    
    
    toc.addEventListener("mouseup",toggleDisplay,1);
  } else {
    
    
    
    document.documentElement.removeChild(document.getElementsByTagName("toc")[0]);
    document.styleSheets[0].cssRules[0].style.marginLeft = "0em";
  }
}

