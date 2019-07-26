



"use strict";

this.EXPORTED_SYMBOLS = ["PageStyle"];

const Ci = Components.interfaces;




this.PageStyle = Object.freeze({
  collect: function (docShell) {
    return PageStyleInternal.collect(docShell);
  },

  restore: function (docShell, frameList, pageStyle) {
    PageStyleInternal.restore(docShell, frameList, pageStyle);
  },
});


const NO_STYLE = "_nostyle";

let PageStyleInternal = {
  



  collect: function (docShell) {
    let markupDocumentViewer =
      docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
    if (markupDocumentViewer.authorStyleDisabled) {
      return NO_STYLE;
    }

    let content = docShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);

    return this.collectFrame(content);
  },

  





  collectFrame: function (content) {
    const forScreen = /(?:^|,)\s*(?:all|screen)\s*(?:,|$)/i;

    let sheets = content.document.styleSheets;
    for (let i = 0; i < sheets.length; i++) {
      let ss = sheets[i];
      let media = ss.media.mediaText;
      if (!ss.disabled && ss.title && (!media || forScreen.test(media))) {
        return ss.title;
      }
    }

    for (let i = 0; i < content.frames.length; i++) {
      let selectedPageStyle = this.collectFrame(content.frames[i]);
      if (selectedPageStyle) {
        return selectedPageStyle;
      }
    }

    return "";
  },

  








  restore: function (docShell, frameList, pageStyle) {
    let disabled = pageStyle == NO_STYLE;

    let markupDocumentViewer =
      docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
    markupDocumentViewer.authorStyleDisabled = disabled;

    for (let [frame, data] of frameList) {
      Array.forEach(frame.document.styleSheets, function(aSS) {
        aSS.disabled = aSS.title && aSS.title != pageStyle;
      });
    }
  },
};
