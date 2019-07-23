





































var elements = [];
var numItems;
var list;
var param;

function selectDialogOnLoad() {
  param = window.arguments[0].QueryInterface( Components.interfaces.nsIDialogParamBlock  );
  if( !param )
  dump( " error getting param block interface\n" );

  var messageText = param.GetString( 1 );
  {
    var messageFragment;

    
    
    var messageParent = (document.getElementById("info.txt"));
    var done = false;
    while (!done) {
      var breakIndex = messageText.indexOf('\n');
      if (breakIndex == 0) {
        
        messageText = messageText.slice(1);
        messageFragment = "";
      } else if (breakIndex > 0) {
        
        messageFragment = messageText.slice(0, breakIndex);

        
        messageText = messageText.slice(breakIndex+1);
      } else {
        
        done = true;
        messageFragment = messageText;
      }
      messageParent.setAttribute("value", messageFragment);
    }
  }

  document.title = param.GetString( 0 );

  list = document.getElementById("list");
  numItems = param.GetInt( 2 );

  var i;
  for ( i = 2; i <= numItems+1; i++ ) {
    var newString = param.GetString( i );
    if (newString == "") {
      newString = "<>";
    }
    elements[i-2] = AppendStringToListbox(list, newString);
  }
  list.selectItem(elements[0]);
  list.focus();

  
  window.sizeToContent();

  
  moveToAlertPosition();
  param.SetInt(0, 1 );
  centerWindowOnScreen();

  
  try {
    const nsISystemSoundService = Components.interfaces.nsISystemSoundService;
    Components.classes["@mozilla.org/systemsoundservice;1"]
              .getService(nsISystemSoundService)
              .playEventSound(nsISystemSoundService.EVENT_SELECT_DIALOG_OPEN);
  } catch (e) { }
}

function commonDialogOnOK() {
  for (var i=0; i<numItems; i++) {
    if (elements[i] == list.selectedItems[0]) {
      param.SetInt(2, i );
      break;
    }
  }
  param.SetInt(0, 0 );
  return true;
}

function commonDialogOnCancel() {
  for (var i=0; i<numItems; i++) {
    if (elements[i]) {
      param.SetInt(2, i );
      break;
    }
  }
  param.SetInt(0, 1 );
  return true;
}

function commonDialogOnDoubleClick() {
  commonDialogOnOK();
  window.close();
}



function AppendStringToListbox(tree, string)
{
  if (tree)
  {
    var listbox = document.getElementById('list');

    var listitem = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "listitem");
    if (listitem)
    {
      listitem.setAttribute("label", string);
      listitem.setAttribute("ondblclick","commonDialogOnDoubleClick()");
      listbox.appendChild(listitem)
      var len = Number(tree.getAttribute("length"));
      if (!len) len = -1;
      tree.setAttribute("length",len+1);
      return listitem;
    }
  }
  return null;
}
