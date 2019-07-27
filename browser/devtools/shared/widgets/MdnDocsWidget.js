























"use strict";

const {Cc, Cu, Ci} = require("chrome");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");



const XHR_PARAMS = "?raw&macros";

var XHR_CSS_URL = "https://developer.mozilla.org/en-US/docs/Web/CSS/";



const PAGE_LINK_PARAMS = "?utm_source=mozilla&utm_medium=firefox-inspector&utm_campaign=default"


var PAGE_LINK_URL = "https://developer.mozilla.org/docs/Web/CSS/";

const BROWSER_WINDOW = 'navigator:browser';













function getMdnPage(pageUrl) {
  let deferred = Promise.defer();

  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);

  xhr.addEventListener("load", onLoaded, false);
  xhr.addEventListener("error", onError, false);

  xhr.open("GET", pageUrl);
  xhr.responseType = "document";
  xhr.send();

  function onLoaded(e) {
    if (xhr.status != 200) {
      deferred.reject({page: pageUrl, status: xhr.status});
    }
    else {
      deferred.resolve(xhr.responseXML);
    }
  }

  function onError(e) {
    deferred.reject({page: pageUrl, status: xhr.status});
  }

  return deferred.promise;
}

















function getCssDocs(cssProperty) {

  let deferred = Promise.defer();
  let pageUrl = XHR_CSS_URL + cssProperty + XHR_PARAMS;

  getMdnPage(pageUrl).then(parseDocsFromResponse, handleRejection);

  function parseDocsFromResponse(responseDocument) {
    let theDocs = {};
    theDocs.summary = getSummary(responseDocument);
    theDocs.syntax = getSyntax(responseDocument);
    if (theDocs.summary || theDocs.syntax) {
      deferred.resolve(theDocs);
    }
    else {
      deferred.reject("Couldn't find the docs in the page.");
    }
  }

  function handleRejection(e) {
    deferred.reject(e.status);
  }

  return deferred.promise;
}

exports.getCssDocs = getCssDocs;

















function MdnDocsWidget(tooltipDocument) {

  
  this.elements = {
    heading: tooltipDocument.getElementById("property-name"),
    summary: tooltipDocument.getElementById("summary"),
    syntax: tooltipDocument.getElementById("syntax"),
    info: tooltipDocument.getElementById("property-info"),
    linkToMdn: tooltipDocument.getElementById("visit-mdn-page")
  };

  
  this.elements.linkToMdn.textContent =
    l10n.strings.GetStringFromName("docsTooltip.visitMDN");

  
  let browserWindow = Services.wm.getMostRecentWindow(BROWSER_WINDOW);
  this.elements.linkToMdn.addEventListener("click", function(e) {
    e.stopPropagation();
    e.preventDefault();
    let link = e.target.href;
    browserWindow.gBrowser.addTab(link);
  });
}

exports.MdnDocsWidget = MdnDocsWidget;

MdnDocsWidget.prototype = {
  



















  loadCssDocs: function(propertyName) {

    




    function initializeDocument(propertyName) {

      
      elements.heading.textContent = propertyName;

      
      elements.linkToMdn.setAttribute("href",
        PAGE_LINK_URL + propertyName + PAGE_LINK_PARAMS);

      
      elements.summary.textContent = "";
      elements.syntax.textContent = "";

      
      elements.info.scrollTop = 0;
      elements.info.scrollLeft = 0;

      
      elements.info.classList.add("devtools-throbber");
    }

    



    function finalizeDocument({summary, syntax}) {
      
      elements.summary.textContent = summary;
      elements.syntax.textContent = syntax;

      
      elements.info.classList.remove("devtools-throbber");

      deferred.resolve(this);
    }

    



    function gotError(error) {
      
      elements.summary.textContent = l10n.strings.GetStringFromName("docsTooltip.loadDocsError");

      
      elements.info.classList.remove("devtools-throbber");

      
      
      deferred.resolve(this);
    }

    let deferred = Promise.defer();
    let elements = this.elements;

    initializeDocument(propertyName);
    getCssDocs(propertyName).then(finalizeDocument, gotError);

    return deferred.promise;
  }
}




function L10N() {}
L10N.prototype = {};

let l10n = new L10N();

loader.lazyGetter(L10N.prototype, "strings", () => {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/inspector.properties");
});







function isAllWhitespace(node) {
  return !(/[^\t\n\r ]/.test(node.textContent));
}







function isIgnorable(node) {
  return (node.nodeType == 8) || 
         ((node.nodeType == 3) && isAllWhitespace(node)); 
}








function nodeAfter(sib) {
  while ((sib = sib.nextSibling)) {
    if (!isIgnorable(sib)) return sib;
  }
  return null;
}














function hasTagName(node, tagName) {
  return node && node.tagName &&
         node.tagName.toLowerCase() == tagName.toLowerCase();
}















function getSummary(mdnDocument) {
  let summary = mdnDocument.getElementById("Summary");
  if (!hasTagName(summary, "H2")) {
    return null;
  }

  let firstParagraph = nodeAfter(summary);
  if (!hasTagName(firstParagraph, "P")) {
    return null;
  }

  return firstParagraph.textContent;
}





















function getSyntax(mdnDocument) {

  let syntax = mdnDocument.getElementById("Syntax");
  if (!hasTagName(syntax, "H2")) {
    return null;
  }

  let firstParagraph = nodeAfter(syntax);
  if (!hasTagName(firstParagraph, "PRE")) {
    return null;
  }

  let secondParagraph = nodeAfter(firstParagraph);
  if (hasTagName(secondParagraph, "PRE")) {
    return secondParagraph.textContent;
  }
  else {
    return firstParagraph.textContent;
  }
}







function setBaseCssDocsUrl(baseUrl) {
  PAGE_LINK_URL = baseUrl;
  XHR_CSS_URL = baseUrl;
}

exports.setBaseCssDocsUrl = setBaseCssDocsUrl;
