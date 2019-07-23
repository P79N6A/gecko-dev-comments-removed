





































var gConsole, gConsoleBundle;



window.onload = function()
{
  gConsole = document.getElementById("ConsoleBox");
  gConsoleBundle = document.getElementById("ConsoleBundle");
  
  top.controllers.insertControllerAt(0, ConsoleController);
  
  updateSortCommand(gConsole.sortOrder);
  updateModeCommand(gConsole.mode);

  var iframe = document.getElementById("Evaluator");
  iframe.addEventListener("load", displayResult, true);
}



function changeMode(aMode)
{
  switch (aMode) {
    case "Errors":
    case "Warnings":
    case "Messages":
      gConsole.mode = aMode;
      break;
    case "All":
      gConsole.mode = null;
  }
  
  document.persist("ConsoleBox", "mode");
}

function clearConsole()
{
  gConsole.clear();
}

function changeSortOrder(aOrder)
{
  updateSortCommand(gConsole.sortOrder = aOrder);
}

function updateSortCommand(aOrder)
{
  var orderString = aOrder == 'reverse' ? "Descend" : "Ascend";
  var bc = document.getElementById("Console:sort"+orderString);
  bc.setAttribute("checked", true);  

  orderString = aOrder == 'reverse' ? "Ascend" : "Descend";
  bc = document.getElementById("Console:sort"+orderString);
  bc.setAttribute("checked", false);
}

function updateModeCommand(aMode)
{
  var bc = document.getElementById("Console:mode" + aMode);
  bc.setAttribute("checked", true);
}

function toggleToolbar(aEl)
{
  var bc = document.getElementById(aEl.getAttribute("observes"));
  var truth = bc.getAttribute("checked");
  bc.setAttribute("checked", truth != "true");
  var toolbar = document.getElementById(bc.getAttribute("_toolbar"));
  toolbar.setAttribute("hidden", truth);

  document.persist(toolbar.id, "hidden");
  document.persist(bc.id, "checked");
}

function copyItemToClipboard()
{
  gConsole.copySelectedItem();
}

function isItemSelected()
{
  return gConsole.selectedItem != null;
}

function UpdateCopyMenu()
{
  goUpdateCommand("cmd_copy");
}

function onEvalKeyPress(aEvent)
{
  if (aEvent.keyCode == 13)
    evaluateTypein();
}

function evaluateTypein()
{
  var code = document.getElementById("TextboxEval").value;
  var evaluator = document.getElementById("Evaluator").contentWindow;
  evaluator.location = "about:blank"; 
  evaluator.location = "javascript: " + encodeURIComponent(code);
}

function displayResult()
{
  var resultRange = Evaluator.document.createRange();
  resultRange.selectNode(Evaluator.document.documentElement);
  var result = resultRange.toString();
  if (result)
    gConsole.mCService.logStringMessage(result);
    
}



var ConsoleController = 
{
  isCommandEnabled: function (aCommand)
  {
    switch (aCommand) {
      case "cmd_copy":
        return isItemSelected();
      default:
        return false;
    }
  },
  
  supportsCommand: function (aCommand) 
  {
    switch (aCommand) {
      case "cmd_copy":
        return true;
      default:
        return false;
    }
  },
  
  doCommand: function (aCommand)
  {
    switch (aCommand) {
      case "cmd_copy":
        copyItemToClipboard();
        break;
      default:
        break;
    }
  },
  
  onEvent: function (aEvent) 
  {
  }
};



function debug(aText)
{
  var csClass = Components.classes['@mozilla.org/consoleservice;1'];
  var cs = csClass.getService(Components.interfaces.nsIConsoleService);
  cs.logStringMessage(aText);
}

function getStackTrace()
{
  var frame = Components.stack.caller;
  var str = "";
  while (frame) {
    if (frame.filename)
      str += frame.filename + ", Line " + frame.lineNumber;
    else
      str += "[" + gConsoleBundle.getString("noFile") + "]";
    
    str += " --> ";
    
    if (frame.functionName)
      str += frame.functionName;
    else
      str += "[" + gConsoleBundle.getString("noFunction") + "]";
      
    str += "\n";
    
    frame = frame.caller;
  }
  
  return str;
}
