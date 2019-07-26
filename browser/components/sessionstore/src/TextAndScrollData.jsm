



"use strict";

this.EXPORTED_SYMBOLS = ["TextAndScrollData"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DocumentUtils",
  "resource:///modules/sessionstore/DocumentUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");




this.TextAndScrollData = Object.freeze({
  updateFrame: function (entry, content, isPinned, options) {
    return TextAndScrollDataInternal.updateFrame(entry, content, isPinned, options);
  },

  restore: function (frameList) {
    TextAndScrollDataInternal.restore(frameList);
  },
});

let TextAndScrollDataInternal = {
  












  updateFrame: function (entry, content, isPinned, options = null) {
    let includePrivateData = options && options.includePrivateData;

    for (let i = 0; i < content.frames.length; i++) {
      if (entry.children && entry.children[i]) {
        this.updateFrame(entry.children[i], content.frames[i], includePrivateData, isPinned);
      }
    }

    let href = (content.parent || content).document.location.href;
    let isHttps = Services.io.newURI(href, null, null).schemeIs("https");
    let topURL = content.top.document.location.href;
    let isAboutSR = this.isAboutSessionRestore(topURL);
    if (includePrivateData || isAboutSR ||
        PrivacyLevel.canSave({isHttps: isHttps, isPinned: isPinned})) {
      let formData = DocumentUtils.getFormData(content.document);

      
      
      
      if (formData && isAboutSR) {
        formData.id["sessionData"] = JSON.parse(formData.id["sessionData"]);
      }

      if (Object.keys(formData.id).length ||
          Object.keys(formData.xpath).length) {
        entry.formdata = formData;
      }

      
      if ((content.document.designMode || "") == "on" && content.document.body) {
        entry.innerHTML = content.document.body.innerHTML;
      }
    }

    
    
    let domWindowUtils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindowUtils);
    let scrollX = {}, scrollY = {};
    domWindowUtils.getScrollXY(false, scrollX, scrollY);
    entry.scroll = scrollX.value + "," + scrollY.value;

    if (topURL == "about:config") {
      entry.formdata = {
        id: { "textbox": content.top.document.getElementById("textbox").value },
        xpath: {}
      };
    }
  },

  isAboutSessionRestore: function (url) {
    return url == "about:sessionrestore" || url == "about:welcomeback";
  },

  restore: function (frameList) {
    for (let [frame, data] of frameList) {
      this.restoreFrame(frame, data);
    }
  },

  restoreFrame: function (content, data) {
    if (data.formdata) {
      let formdata = data.formdata;

      
      
      if (!("xpath" in formdata || "id" in formdata)) {
        formdata = { xpath: {}, id: {} };

        for each (let [key, value] in Iterator(data.formdata)) {
          if (key.charAt(0) == "#") {
            formdata.id[key.slice(1)] = value;
          } else {
            formdata.xpath[key] = value;
          }
        }
      }

      
      
      
      if (this.isAboutSessionRestore(data.url) &&
          "sessionData" in formdata.id &&
          typeof formdata.id["sessionData"] == "object") {
        formdata.id["sessionData"] = JSON.stringify(formdata.id["sessionData"]);
      }

      
      data.formdata = formdata;
      
      DocumentUtils.mergeFormData(content.document, formdata);
    }

    if (data.innerHTML) {
      
      
      
      let savedURL = content.document.location.href;

      setTimeout(function() {
        if (content.document.designMode == "on" &&
            content.document.location.href == savedURL &&
            content.document.body) {
          content.document.body.innerHTML = data.innerHTML;
        }
      }, 0);
    }

    let match;
    if (data.scroll && (match = /(\d+),(\d+)/.exec(data.scroll)) != null) {
      content.scrollTo(match[1], match[2]);
    }
  },
};
