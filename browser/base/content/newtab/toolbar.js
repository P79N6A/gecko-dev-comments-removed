#ifdef 0



#endif





let gToolbar = {
  



  init: function Toolbar_init(aSelector) {
    this._node = document.querySelector(aSelector);
    let buttons = this._node.querySelectorAll("input");

    
    ["show", "hide", "reset"].forEach(function (aType, aIndex) {
      let self = this;
      let button = buttons[aIndex];
      let handler = function () self[aType]();

      button.addEventListener("click", handler, false);

#ifdef XP_MACOSX
      
      
      
      
      button.addEventListener("mousedown", function () {
        window.focus();
      }, false);
#endif
    }, this);
  },

  


  show: function Toolbar_show() {
    this._passButtonFocus("show", "hide");
    gAllPages.enabled = true;
  },

  


  hide: function Toolbar_hide() {
    this._passButtonFocus("hide", "show");
    gAllPages.enabled = false;
  },

  



  reset: function Toolbar_reset(aCallback) {
    this._passButtonFocus("reset", "hide");
    let node = gGrid.node;

    
    gTransformation.fadeNodeOut(node, function () {
      NewTabUtils.reset();

      gLinks.populateCache(function () {
        gAllPages.update();

        
        setTimeout(function () gTransformation.fadeNodeIn(node, aCallback));
      });
    });
  },

  




  _passButtonFocus: function Toolbar_passButtonFocus(aCurrent, aNext) {
    if (document.querySelector("#toolbar-button-" + aCurrent + ":-moz-focusring"))
      document.getElementById("toolbar-button-" + aNext).focus();
  }
};

