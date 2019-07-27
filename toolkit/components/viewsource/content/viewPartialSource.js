





Components.utils.import("resource://gre/modules/Services.jsm");

function onLoadViewPartialSource() {
  
  
  let wrapLongLines = Services.prefs.getBoolPref("view_source.wrap_long_lines");
  document.getElementById("menu_wrapLongLines")
          .setAttribute("checked", wrapLongLines);
  document.getElementById("menu_highlightSyntax")
          .setAttribute("checked",
                        Services.prefs.getBoolPref("view_source.syntax_highlight"));

  if (window.arguments[3] == 'selection')
    viewSourceChrome.loadViewSourceFromSelection(window.arguments[2]);
  else
    viewSourceChrome.loadViewSourceFromFragment(window.arguments[2], window.arguments[3]);

  window.content.focus();
}
