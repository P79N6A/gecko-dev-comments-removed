



"use strict";

this.EXPORTED_SYMBOLS = ["PageStyle"];

const Ci = Components.interfaces;




this.PageStyle = Object.freeze({
  collect: function (docShell, frameTree) {
    return PageStyleInternal.collect(docShell, frameTree);
  },

  restoreTree: function (docShell, data) {
    PageStyleInternal.restoreTree(docShell, data);
  }
});


const NO_STYLE = "_nostyle";

let PageStyleInternal = {
  


  collect: function (docShell, frameTree) {
    let result = frameTree.map(({document: doc}) => {
      let style;

      if (doc) {
        
        style = doc.selectedStyleSheetSet || doc.lastStyleSheetSet;
      }

      return style ? {pageStyle: style} : null;
    });

    let markupDocumentViewer =
      docShell.contentViewer;

    if (markupDocumentViewer.authorStyleDisabled) {
      result = result || {};
      result.disabled = true;
    }

    return result && Object.keys(result).length ? result : null;
  },

  



















  restoreTree: function (docShell, data) {
    let disabled = data.disabled || false;
    let markupDocumentViewer =
      docShell.contentViewer;
    markupDocumentViewer.authorStyleDisabled = disabled;

    function restoreFrame(root, data) {
      if (data.hasOwnProperty("pageStyle")) {
        root.document.selectedStyleSheetSet = data.pageStyle;
      }

      if (!data.hasOwnProperty("children")) {
        return;
      }

      let frames = root.frames;
      data.children.forEach((child, index) => {
        if (child && index < frames.length) {
          restoreFrame(frames[index], child);
        }
      });
    }

    let ifreq = docShell.QueryInterface(Ci.nsIInterfaceRequestor);
    restoreFrame(ifreq.getInterface(Ci.nsIDOMWindow), data);
  }
};
