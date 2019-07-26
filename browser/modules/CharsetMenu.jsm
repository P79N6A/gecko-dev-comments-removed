



this.EXPORTED_SYMBOLS = [ "CharsetMenu" ];

const { classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyGetter(this, "gBundle", function() {
  const kUrl = "chrome://browser/locale/charsetMenu.properties";
  return Services.strings.createBundle(kUrl);
});








const kEncodings = new Set([
  
  "UTF-8",
  "windows-1252",
  
  "windows-1256",
  "ISO-8859-6",
  
  "windows-1257",
  "ISO-8859-4",
  
  
  "windows-1250",
  "ISO-8859-2",
  
  "gbk",
  "gb18030",
  
  "Big5",
  
  "windows-1251",
  "ISO-8859-5",
  "KOI8-R",
  "KOI8-U",
  "IBM866", 
  
  
  "windows-1253",
  "ISO-8859-7",
  
  "windows-1255",
  "ISO-8859-8",
  
  "Shift_JIS",
  "EUC-JP",
  "ISO-2022-JP",
  
  "EUC-KR",
  
  "windows-874",
  
  "windows-1254",
  
  "windows-1258",
  
  
  
  
  
  
  
  
]);


const kPinned = [
  "UTF-8",
  "windows-1252"
];

this.CharsetMenu = Object.freeze({
  build: function BuildCharsetMenu(event, idPrefix="", showAccessKeys=false) {
    let parent = event.target;
    if (parent.lastChild.localName != "menuseparator") {
      
      return;
    }
    let doc = parent.ownerDocument;

    function createItem(encoding) {
      let menuItem = doc.createElement("menuitem");
      menuItem.setAttribute("type", "radio");
      menuItem.setAttribute("name", "charsetGroup");
      try {
        menuItem.setAttribute("label", gBundle.GetStringFromName(encoding));
      } catch (e) {
        
        menuItem.setAttribute("label", encoding);
      }
      if (showAccessKeys) {
        try {
          menuItem.setAttribute("accesskey",
                                gBundle.GetStringFromName(encoding + ".key"));
        } catch (e) {
          
        }
      }
      menuItem.setAttribute("id", idPrefix + "charset." + encoding);
      return menuItem;
    }

    
    
    let encodings = new Set(kEncodings);
    for (let encoding of kPinned) {
      encodings.delete(encoding);
      parent.appendChild(createItem(encoding));
    }
    parent.appendChild(doc.createElement("menuseparator"));
    let list = [];
    for (let encoding of encodings) {
      list.push(createItem(encoding));
    }

    list.sort(function (a, b) {
      let titleA = a.getAttribute("label");
      let titleB = b.getAttribute("label");
      
      
      let index;
      if ((index = titleA.indexOf("(")) > -1) {
        titleA = titleA.substring(0, index);
      }
      if ((index = titleB.indexOf("(")) > -1) {
        titleA = titleB.substring(0, index);
      }
      let comp = titleA.localeCompare(titleB);
      if (comp) {
        return comp;
      }
      
      
      
      let idA = a.getAttribute("id");
      let idB = b.getAttribute("id");
      if (idA < idB) {
        return 1;
      }
      if (idB < idA) {
        return -1;
      }
      return 0;
    });

    for (let item of list) {
      parent.appendChild(item);
    }
  },
});
