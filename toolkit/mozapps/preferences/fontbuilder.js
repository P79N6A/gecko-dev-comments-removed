





var FontBuilder = {
  _enumerator: null,
  get enumerator ()
  {
    if (!this._enumerator) {
      this._enumerator = Components.classes["@mozilla.org/gfx/fontenumerator;1"]
                                   .createInstance(Components.interfaces.nsIFontEnumerator);
    }
    return this._enumerator;
  },

  _allFonts: null,
  buildFontList: function (aLanguage, aFontType, aMenuList) 
  {
    
    while (aMenuList.hasChildNodes())
      aMenuList.removeChild(aMenuList.firstChild);
    
    var defaultFont = null;
    
    var fonts = this.enumerator.EnumerateFonts(aLanguage, aFontType, { } );
    if (fonts.length > 0)
      defaultFont = this.enumerator.getDefaultFont(aLanguage, aFontType);
    else {
      fonts = this.enumerator.EnumerateFonts(aLanguage, "", { });
      if (fonts.length > 0)
        defaultFont = this.enumerator.getDefaultFont(aLanguage, "");
    }
    
    if (!this._allFonts)
      this._allFonts = this.enumerator.EnumerateAllFonts({});
    
    
    var popup = document.createElement("menupopup");
    var separator;
    if (fonts.length > 0) {
      if (defaultFont) {
        var bundlePreferences = document.getElementById("bundlePreferences");
        var label = bundlePreferences.getFormattedString("labelDefaultFont", [defaultFont]);
        var menuitem = document.createElement("menuitem");
        menuitem.setAttribute("label", label);
        menuitem.setAttribute("value", ""); 
        popup.appendChild(menuitem);
        
        separator = document.createElement("menuseparator");
        popup.appendChild(separator);
      }
      
      for (var i = 0; i < fonts.length; ++i) {
        menuitem = document.createElement("menuitem");
        menuitem.setAttribute("value", fonts[i]);
        menuitem.setAttribute("label", fonts[i]);
        popup.appendChild(menuitem);
      }
    }
    
    
    if (this._allFonts.length > fonts.length) {
      
      
      
      var builtItem = separator ? separator.nextSibling : popup.firstChild;
      var builtItemValue = builtItem ? builtItem.getAttribute("value") : null;

      separator = document.createElement("menuseparator");
      popup.appendChild(separator);
      
      for (i = 0; i < this._allFonts.length; ++i) {
        if (this._allFonts[i] != builtItemValue) {
          menuitem = document.createElement("menuitem");
          menuitem.setAttribute("value", this._allFonts[i]);
          menuitem.setAttribute("label", this._allFonts[i]);
          popup.appendChild(menuitem);
        }
        else {
          builtItem = builtItem.nextSibling;
          builtItemValue = builtItem ? builtItem.getAttribute("value") : null;
        }
      }
    }
    aMenuList.appendChild(popup);    
  },

  readFontSelection(aElement)
  {
    
    
    
    
    let preference = document.getElementById(aElement.getAttribute("preference"));
    if (preference.value) {
      let fontItems = aElement.getElementsByAttribute("value", preference.value);

      
      if (fontItems.length)
        return undefined;
    }

    let defaultValue = aElement.firstChild.firstChild.getAttribute("value");
    let fontNameList = preference.name.replace(".name.", ".name-list.");
    let prefFontNameList = document.getElementById(fontNameList);
    if (!prefFontNameList || !prefFontNameList.value)
      return defaultValue;

    let fontNames = prefFontNameList.value.split(",");

    for (let i = 0; i < fontNames.length; ++i) {
      let fontName = this.enumerator.getStandardFamilyName(fontNames[i].trim());
      let fontItems = aElement.getElementsByAttribute("value", fontName);
      if (fontItems.length)
        return fontItems[0].getAttribute("value");
    }
    return defaultValue;
  }
};
