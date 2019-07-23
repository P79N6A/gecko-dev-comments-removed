













































var gViewer = window.arguments[0];
var gTarget = window.arguments[1];




function execute()
{
  var txf = document.getElementById("txfExprInput");
  var rad = document.getElementById("inspect-new-window");
  try {
    gViewer.doEvalExpr(txf.value, gTarget, rad.selected);
  } catch (ex) {
    
    Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
              .getService(Components.interfaces.nsIPromptService)
                .alert(window, document.getElementById("strings")
                               .getString("jsObjectExpressionError.title"), ex);
    return false;
  }
  return true;
}
