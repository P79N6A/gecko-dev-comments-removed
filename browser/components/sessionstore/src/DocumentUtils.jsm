



EXPORTED_SYMBOLS = [ "DocumentUtils" ];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/sessionstore/XPathGenerator.jsm");

let DocumentUtils = {
  













  getFormData: function DocumentUtils_getFormData(aDocument) {
    let formNodes = aDocument.evaluate(
      XPathGenerator.restorableFormNodes,
      aDocument,
      XPathGenerator.resolveNS,
      Ci.nsIDOMXPathResult.UNORDERED_NODE_ITERATOR_TYPE, null
    );

    let node;
    let ret = {id: {}, xpath: {}};

    
    
    const MAX_TRAVERSED_XPATHS = 100;
    let generatedCount = 0;

    while (node = formNodes.iterateNext()) {
      let nId = node.id;
      let hasDefaultValue = true;
      let value;

      
      
      if (!nId && generatedCount > MAX_TRAVERSED_XPATHS) {
        continue;
      }

      if (node instanceof Ci.nsIDOMHTMLInputElement ||
          node instanceof Ci.nsIDOMHTMLTextAreaElement) {
        switch (node.type) {
          case "checkbox":
          case "radio":
            value = node.checked;
            hasDefaultValue = value == node.defaultChecked;
            break;
          case "file":
            value = { type: "file", fileList: node.mozGetFileNameArray() };
            hasDefaultValue = !value.fileList.length;
            break;
          default: 
            value = node.value;
            hasDefaultValue = value == node.defaultValue;
            break;
        }
      } else if (!node.multiple) {
        
        
        hasDefaultValue = false;
        value = { selectedIndex: node.selectedIndex, value: node.value };
      } else {
        
        
        let options = Array.map(node.options, function(aOpt, aIx) {
          let oSelected = aOpt.selected;
          hasDefaultValue = hasDefaultValue && (oSelected == aOpt.defaultSelected);
          return oSelected ? aOpt.value : -1;
        });
        value = options.filter(function(aIx) aIx !== -1);
      }

      
      
      if (!hasDefaultValue) {
        if (nId) {
          ret.id[nId] = value;
        } else {
          generatedCount++;
          ret.xpath[XPathGenerator.generate(node)] = value;
        }
      }
    }

    return ret;
  },

  
















  mergeFormData: function DocumentUtils_mergeFormData(aDocument, aData) {
    if ("xpath" in aData) {
      for each (let [xpath, value] in Iterator(aData.xpath)) {
        let node = XPathGenerator.resolve(aDocument, xpath);

        if (node) {
          this.restoreFormValue(node, value, aDocument);
        }
      }
    }

    if ("id" in aData) {
      for each (let [id, value] in Iterator(aData.id)) {
        let node = aDocument.getElementById(id);

        if (node) {
          this.restoreFormValue(node, value, aDocument);
        }
      }
    }
  },

  















  restoreFormValue: function DocumentUtils_restoreFormValue(aNode, aValue, aDocument) {
    aDocument = aDocument || aNode.ownerDocument;

    let eventType;

    if (typeof aValue == "string" && aNode.type != "file") {
      
      if (aNode.value == aValue) {
        return;
      }

      aNode.value = aValue;
      eventType = "input";
    } else if (typeof aValue == "boolean") {
      
      if (aNode.checked == aValue) {
        return;
      }

      aNode.checked = aValue;
      eventType = "change";
    } else if (typeof aValue == "number") {
      
      
      
      if (aNode.selectedIndex == aValue) {
        return;
      }

      if (aValue < aNode.options.length) {
        aNode.selectedIndex = aValue;
        eventType = "change";
      }
    } else if (aValue && aValue.selectedIndex >= 0 && aValue.value) {
      

      
      if (aNode.options[aNode.selectedIndex].value == aValue.value) {
        return;
      }

      
      for (let i = 0; i < aNode.options.length; i++) {
        if (aNode.options[i].value == aValue.value) {
          aNode.selectedIndex = i;
          break;
        }
      }
      eventType = "change";
    } else if (aValue && aValue.fileList && aValue.type == "file" &&
      aNode.type == "file") {
      aNode.mozSetFileNameArray(aValue.fileList, aValue.fileList.length);
      eventType = "input";
    } else if (aValue && typeof aValue.indexOf == "function" && aNode.options) {
      Array.forEach(aNode.options, function(opt, index) {
        
        opt.selected = aValue.indexOf(opt.value) > -1;

        
        if (!opt.defaultSelected) {
          eventType = "change";
        }
      });
    }

    
    if (eventType) {
      let event = aDocument.createEvent("UIEvents");
      event.initUIEvent(eventType, true, true, aDocument.defaultView, 0);
      aNode.dispatchEvent(event);
    }
  }
};
