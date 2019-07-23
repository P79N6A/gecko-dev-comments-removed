




































function inspectDOMDocument(aDocument, aModal)
{
  window.openDialog("chrome://inspector/content/", "_blank", 
              "chrome,all,dialog=no"+(aModal?",modal":""), aDocument);
}

function inspectDOMNode(aNode, aModal)
{
  window.openDialog("chrome://inspector/content/", "_blank", 
              "chrome,all,dialog=no"+(aModal?",modal":""), aNode);
}

function inspectObject(aObject, aModal)
{
  window.openDialog("chrome://inspector/content/object.xul", "_blank", 
              "chrome,all,dialog=no"+(aModal?",modal":""), aObject);
}
