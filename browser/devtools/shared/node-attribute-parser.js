





"use strict";



















const TYPE_STRING = "string";
const TYPE_URI = "uri";
const TYPE_URI_LIST = "uriList";
const TYPE_IDREF = "idref";
const TYPE_IDREF_LIST = "idrefList";
const TYPE_RESOURCE_URI = "resource";

const SVG_NS = "http://www.w3.org/2000/svg";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const HTML_NS = "http://www.w3.org/1999/xhtml";

const ATTRIBUTE_TYPES = [
  {namespaceURI: HTML_NS, attributeName: "action", tagName: "form", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "background", tagName: "body", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "cite", tagName: "blockquote", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "cite", tagName: "q", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "cite", tagName: "del", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "cite", tagName: "ins", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "classid", tagName: "object", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "codebase", tagName: "object", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "codebase", tagName: "applet", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "command", tagName: "menuitem", type: TYPE_IDREF},
  {namespaceURI: "*", attributeName: "contextmenu", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "data", tagName: "object", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "for", tagName: "label", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "for", tagName: "output", type: TYPE_IDREF_LIST},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "button", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "fieldset", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "input", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "keygen", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "label", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "object", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "output", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "select", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "form", tagName: "textarea", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "formaction", tagName: "button", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "formaction", tagName: "input", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "headers", tagName: "td", type: TYPE_IDREF_LIST},
  {namespaceURI: HTML_NS, attributeName: "headers", tagName: "th", type: TYPE_IDREF_LIST},
  {namespaceURI: HTML_NS, attributeName: "href", tagName: "a", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "href", tagName: "area", type: TYPE_URI},
  {namespaceURI: "*", attributeName: "href", tagName: "link", type: TYPE_RESOURCE_URI,
   isValid: (namespaceURI, tagName, attributes) => {
    return getAttribute(attributes, "rel") === "stylesheet";
   }},
  {namespaceURI: "*", attributeName: "href", tagName: "link", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "href", tagName: "base", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "icon", tagName: "menuitem", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "list", tagName: "input", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "longdesc", tagName: "img", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "longdesc", tagName: "frame", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "longdesc", tagName: "iframe", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "manifest", tagName: "html", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "menu", tagName: "button", type: TYPE_IDREF},
  {namespaceURI: HTML_NS, attributeName: "ping", tagName: "a", type: TYPE_URI_LIST},
  {namespaceURI: HTML_NS, attributeName: "ping", tagName: "area", type: TYPE_URI_LIST},
  {namespaceURI: HTML_NS, attributeName: "poster", tagName: "video", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "profile", tagName: "head", type: TYPE_URI},
  {namespaceURI: "*", attributeName: "src", tagName: "script", type: TYPE_RESOURCE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "input", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "frame", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "iframe", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "img", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "audio", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "embed", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "source", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "track", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "src", tagName: "video", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "usemap", tagName: "img", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "usemap", tagName: "input", type: TYPE_URI},
  {namespaceURI: HTML_NS, attributeName: "usemap", tagName: "object", type: TYPE_URI},
  {namespaceURI: "*", attributeName: "xmlns", tagName: "*", type: TYPE_URI},
  {namespaceURI: XUL_NS, attributeName: "command", tagName: "key", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "containment", tagName: "*", type: TYPE_URI},
  {namespaceURI: XUL_NS, attributeName: "context", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "datasources", tagName: "*", type: TYPE_URI_LIST},
  {namespaceURI: XUL_NS, attributeName: "insertafter", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "insertbefore", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "menu", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "observes", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "popup", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "ref", tagName: "*", type: TYPE_URI},
  {namespaceURI: XUL_NS, attributeName: "removeelement", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "sortResource", tagName: "*", type: TYPE_URI},
  {namespaceURI: XUL_NS, attributeName: "sortResource2", tagName: "*", type: TYPE_URI},
  {namespaceURI: XUL_NS, attributeName: "src", tagName: "stringbundle", type: TYPE_URI},
  {namespaceURI: XUL_NS, attributeName: "template", tagName: "*", type: TYPE_IDREF},
  {namespaceURI: XUL_NS, attributeName: "tooltip", tagName: "*", type: TYPE_IDREF},
  
  
  
  
  
  
  
];

let parsers = {
  [TYPE_URI]: function(attributeValue) {
    return [{
      type: TYPE_URI,
      value: attributeValue
    }];
  },
  [TYPE_URI_LIST]: function(attributeValue) {
    let data = splitBy(attributeValue, " ");
    for (let token of data) {
      if (!token.type) {
        token.type = TYPE_URI;
      }
    }
    return data;
  },
  [TYPE_RESOURCE_URI]: function(attributeValue) {
    return [{
      type: TYPE_RESOURCE_URI,
      value: attributeValue
    }];
  },
  [TYPE_IDREF]: function(attributeValue) {
    return [{
      type: TYPE_IDREF,
      value: attributeValue
    }];
  },
  [TYPE_IDREF_LIST]: function(attributeValue) {
    let data = splitBy(attributeValue, " ");
    for (let token of data) {
      if (!token.type) {
        token.type = TYPE_IDREF;
      }
    }
    return data;
  }
};


















function parseAttribute(namespaceURI, tagName, attributes, attributeName) {
  if (!hasAttribute(attributes, attributeName)) {
    throw new Error(`Attribute ${attributeName} isn't part of the provided attributes`);
  }

  let type = getType(namespaceURI, tagName, attributes, attributeName);
  if (!type) {
    return [{
      type: TYPE_STRING,
      value: getAttribute(attributes, attributeName)
    }];
  }

  return parsers[type](getAttribute(attributes, attributeName));
}











function getType(namespaceURI, tagName, attributes, attributeName) {
  for (let typeData of ATTRIBUTE_TYPES) {
    let hasAttribute = attributeName === typeData.attributeName ||
                       typeData.attributeName === "*";
    let hasNamespace = namespaceURI === typeData.namespaceURI ||
                       typeData.namespaceURI === "*";
    let hasTagName = tagName.toLowerCase() === typeData.tagName ||
                     typeData.tagName === "*";
    let isValid = typeData.isValid
                  ? typeData.isValid(namespaceURI, tagName, attributes, attributeName)
                  : true;

    if (hasAttribute && hasNamespace && hasTagName && isValid) {
      return typeData.type;
    }
  }

  return null;
}

function getAttribute(attributes, attributeName) {
  for (let {name, value} of attributes) {
    if (name === attributeName) {
      return value;
    }
  }
  return null;
}

function hasAttribute(attributes, attributeName) {
  for (let {name, value} of attributes) {
    if (name === attributeName) {
      return true;
    }
  }
  return false;
}









function splitBy(value, splitChar) {
  let data = [], i = 0, buffer = "";
  while (i <= value.length) {
    if (i === value.length && buffer) {
      data.push({value: buffer});
    }
    if (value[i] === splitChar) {
      if (buffer) {
        data.push({value: buffer});
      }
      data.push({
        type: TYPE_STRING,
        value: splitChar
      });
      buffer = "";
    } else {
      buffer += value[i];
    }

    i ++;
  }
  return data;
}

exports.parseAttribute = parseAttribute;

exports.splitBy = splitBy;
