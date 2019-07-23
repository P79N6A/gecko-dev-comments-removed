














































var dialog;



const gCheckBoxIds = {
  cbxStateHover: 4,
  cbxStateActive: 1,
  cbxStateFocus: 2
};


window.addEventListener("load", PseudoClassDialog_initialize, false);

function PseudoClassDialog_initialize()
{
  dialog = new PseudoClassDialog();
  dialog.initialize();
}




function PseudoClassDialog() 
{
  this.mOpener = window.opener.viewer;
  this.mSubject = window.arguments[0];

  this.mDOMUtils = XPCU.getService("@mozilla.org/inspector/dom-utils;1", "inIDOMUtils");
}

PseudoClassDialog.prototype = 
{
  
  initialize: function()
  {
    var state = this.mDOMUtils.getContentState(this.mSubject);
    
    for (var key in gCheckBoxIds) {
      if (gCheckBoxIds[key] & state) {
        var cbx = document.getElementById(key);
        cbx.setAttribute("checked", "true");
      }
    }
  },
  
  onOk: function()
  {
    var el = this.mSubject;
    var root = el.ownerDocument.documentElement;
    
    for (var key in gCheckBoxIds) {
      var cbx = document.getElementById(key);
      if (cbx.checked) 
        this.mDOMUtils.setContentState(el, gCheckBoxIds[key]);
      else
        this.mDOMUtils.setContentState(root, gCheckBoxIds[key]);
    }
  }
  
};

