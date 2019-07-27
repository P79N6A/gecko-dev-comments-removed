

function sort(collection, key)
{
  var i, j;
  var count = collection.length;
  var parent, child;
 
  for (i = count-1; i >= 0; i--) {
    for (j = 1; j <= i; j++) {
      if (collection[j-1][key] > collection[j][key]) {
         
         
         child = collection[j];
         parent = child.parentNode;

         collection[j] = collection[j-1];
         collection[j-1] = child;

         parent.removeChild(child);       
         parent.insertBefore(child, collection[j]);
      }
    }
  }
}






function collectInfo(nodes, propNames)
{
  var i, j, k;
  var ncount = nodes.length; 
  var pcount = propNames.length;

  for (i = 0; i < ncount; i++) {
    var node = nodes[i];
    var childNodes = node.childNodes;
    var ccount = childNodes.length;
 
    for (j = 0; j < ccount; j++) {
      var child = childNodes[j];

      if (child.nodeType == Node.ELEMENT_NODE) {
        var tagName = child.tagName;

        for (k = 0; k < pcount; k++) {
          var prop = propNames[k];
          if (prop == tagName) {
            node[prop] = child.firstChild.data;
          }  
        }
      }    
    }
  }
}

var enabled = true;
function toggleStyleSheet()
{
  if (enabled) {
    document.styleSheets[2].disabled = true;
  }
  else {
    document.styleSheets[2].disabled = false;
  }

  enabled = !enabled;
}





function initiateToggle()
{
  setTimeout(toggleStyleSheet, 0);
}

var sortableProps = new Array("Author", "Title", "ISBN");
var books = new Array();



var bookset = document.getElementsByTagName("Book");




for (var i=0; i < bookset.length; i++) {
  books[i] = bookset[i];
}

collectInfo(books, sortableProps);

