






































var browser;
var dialog;

function onLoad() {
	dialog = new Object;
	dialog.input     = document.getElementById( "dialog.input" );
    dialog.ok        = document.getElementById( "dialog.ok" );
    dialog.test        = document.getElementById( "dialog.test" );
    dialog.testLabel        = document.getElementById( "dialog.testLabel" );
    dialog.cancel    = document.getElementById( "dialog.cancel" );
    dialog.help      = document.getElementById( "dialog.help" );
	dialog.newWindow = document.getElementById( "dialog.newWindow" );

	browser = XPAppCoresManager.Find( window.arguments[0] );
	if ( !browser ) {
		dump( "unable to get browser app core\n" );
        window.close();
        return;
	}

	
	dialog.input.focus();
}

function onTyping( key ) {
   
   if ( key == 13 ) {
      
      if ( !dialog.ok.disabled ) {
         open();
      }
   } else {
      
      if ( dialog.input.value == "" ) {
         
         if ( !dialog.ok.disabled ) {
            dialog.ok.setAttribute( "disabled", "" );
         }
      } else {
         
         if ( dialog.ok.disabled ) {
            dialog.ok.removeAttribute( "disabled" );
         }
      }
   }
}

function open() {
   if ( dialog.ok.disabled ) {
      return;
   }

	var url = dialog.input.value;

	if ( !dialog.newWindow.checked ) {
		
		browser.loadUrl( url );
	} else {
		
        window.openDialog( "chrome://navigator/content/", "_blank", "chrome,dialog=no,all", url );
	}

	
    window.close();
}

function choose() {
	
	browser.openWindow();
    window.close();
}

function cancel() {
    window.close();
}

function help() {
    if ( dialog.help.disabled ) {
        return;
    }
    dump( "openLocation::help() not implemented\n" );
}

function strresTest() {
  var Bundle = srGetStrBundle("resource://gre/res/strres.properties");
  var	ostr1 = Bundle.GetStringFromName("file");
  dump("\n--** JS strBundle GetStringFromName file=" + ostr1 +
      "len=" + ostr1.length + "**--\n");
  var	ostr2 = Bundle.GetStringFromID(123);
  dump("\n--** JS strBundle GetStringFromID 123=" + ostr2 + 
      "len=" + ostr2.length + "**--\n");

  var	ostr3 = Bundle.GetStringFromName("loyal");
  dump("\n--** JS strBundle GetStringFromName loyal=" + ostr3 + 
       "len=" + ostr3.length + "**--\n");
  
  var	ostr4 = Bundle.GetStringFromName("trout");
  dump("\n--** JS strBundle GetStringFromName eutf8=" + ostr4 +
       "len=" + ostr4.length + "**--\n");
  var	ostr5 = "\u88e6\ue3bb\u8b82"; 
  dump("\n--** JS strBundle GetStringFromName escaped=" + ostr5 +
       "len=" + ostr5.length + "**--\n");


  
  dialog.ok.setAttribute("value", ostr2); 
  dialog.test.setAttribute("value", ostr3); 
  dialog.cancel.setAttribute("value", ostr4); 
}
