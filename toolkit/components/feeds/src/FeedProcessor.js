






































function LOG(str) {
  dump("*** " + str + "\n");
}

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const IO_CONTRACTID = "@mozilla.org/network/io-service;1"
const BAG_CONTRACTID = "@mozilla.org/hash-property-bag;1"
const ARRAY_CONTRACTID = "@mozilla.org/array;1";
const SAX_CONTRACTID = "@mozilla.org/saxparser/xmlreader;1";
const UNESCAPE_CONTRACTID = "@mozilla.org/feed-unescapehtml;1";

var gIoService = Cc[IO_CONTRACTID].getService(Ci.nsIIOService);
var gUnescapeHTML = Cc[UNESCAPE_CONTRACTID].
                    getService(Ci.nsIScriptableUnescapeHTML);

const XMLNS = "http://www.w3.org/XML/1998/namespace";
const RSS090NS = "http://my.netscape.com/rdf/simple/0.9/";
const WAIROLE_NS = "http://www.w3.org/2005/01/wai-rdf/GUIRoleTaxonomy#";


function strToURI(link, base) {
  var base = base || null;
  try {
    return gIoService.newURI(link, null, base);
  }
  catch(e) {
    return null;
  }
}

function isArray(a) {
  return isObject(a) && a.constructor == Array;
}

function isObject(a) {
  return (a && typeof a == "object") || isFunction(a);
}

function isFunction(a) {
  return typeof a == "function";
}

function isIID(a, iid) {
  var rv = false;
  try {
    a.QueryInterface(iid);
    rv = true;
  }
  catch(e) {
  }
  return rv;
}

function isIArray(a) {
  return isIID(a, Ci.nsIArray);
}

function isIFeedContainer(a) {
  return isIID(a, Ci.nsIFeedContainer);
}

function stripTags(someHTML) {
  return someHTML.replace(/<[^>]+>/g,"");
}





const IANA_URI = "http://www.iana.org/assignments/relation/";
function findAtomLinks(rel, links) {
  var rvLinks = [];
  for (var i = 0; i < links.length; ++i) {
    var linkElement = links.queryElementAt(i, Ci.nsIPropertyBag2);
    
    if (bagHasKey(linkElement, "href")) {
      var relAttribute = null;
      if (bagHasKey(linkElement, "rel"))
        relAttribute = linkElement.getPropertyAsAString("rel")
      if ((!relAttribute && rel == "alternate") || relAttribute == rel) {
        rvLinks.push(linkElement);
        continue;
      }
      
      if (relAttribute == IANA_URI + rel) {
        rvLinks.push(linkElement);
      }
    }
  }
  return rvLinks;
}

function xmlEscape(s) {
  s = s.replace(/&/g, "&amp;");
  s = s.replace(/>/g, "&gt;");
  s = s.replace(/</g, "&lt;");
  s = s.replace(/"/g, "&quot;");
  s = s.replace(/'/g, "&apos;");
  return s;
}

function arrayContains(array, element) {
  for (var i = 0; i < array.length; ++i) {
    if (array[i] == element) {
      return true;
    }
  }
  return false;
}


function bagHasKey(bag, key) {
  try {
    bag.getProperty(key);
    return true;
  }
  catch (e) {
    return false;
  }
}

function makePropGetter(key) {
  return function FeedPropGetter(bag) {
    try {
      return value = bag.getProperty(key);
    }
    catch(e) {
    }
    return null;
  }
}















const HOURS_TO_MINUTES = 60;
const MINUTES_TO_SECONDS = 60;
const SECONDS_TO_MILLISECONDS = 1000;
const MINUTES_TO_MILLISECONDS = MINUTES_TO_SECONDS * SECONDS_TO_MILLISECONDS;
const HOURS_TO_MILLISECONDS = HOURS_TO_MINUTES * MINUTES_TO_MILLISECONDS;
function W3CToIETFDate(dateString) {

  var parts = dateString.match(/(\d\d\d\d)(-(\d\d))?(-(\d\d))?(T(\d\d):(\d\d)(:(\d\d)(\.(\d+))?)?(Z|([+-])(\d\d):(\d\d))?)?/);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  var date = new Date(parts[1], parts[3] - 1, parts[5], parts[7] || 0,
                      parts[8] || 0, parts[10] || 0, parts[12] || 0);

  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  

  
  
  

  
  var remoteToUTCOffset = 0;
  if (parts[13] && parts[13] != "Z") {
    var direction = (parts[14] == "+" ? 1 : -1);
    if (parts[15])
      remoteToUTCOffset += direction * parts[15] * HOURS_TO_MILLISECONDS;
    if (parts[16])
      remoteToUTCOffset += direction * parts[16] * MINUTES_TO_MILLISECONDS;
  }
  remoteToUTCOffset = remoteToUTCOffset * -1; 

  
  var UTCToLocalOffset = date.getTimezoneOffset() * MINUTES_TO_MILLISECONDS;
  UTCToLocalOffset = UTCToLocalOffset * -1; 
  date.setTime(date.getTime() + remoteToUTCOffset + UTCToLocalOffset);

  return date.toUTCString();
}

const RDF_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";

var gNamespaces = {
  "http://webns.net/mvcb/":"admin",
  "http://backend.userland.com/rss":"",
  "http://blogs.law.harvard.edu/tech/rss":"",
  "http://www.w3.org/2005/Atom":"atom",
  "http://purl.org/atom/ns#":"atom03",
  "http://purl.org/rss/1.0/modules/content/":"content",
  "http://purl.org/dc/elements/1.1/":"dc",
  "http://purl.org/dc/terms/":"dcterms",
  "http://www.w3.org/1999/02/22-rdf-syntax-ns#":"rdf",
  "http://purl.org/rss/1.0/":"rss1",
  "http://my.netscape.com/rdf/simple/0.9/":"rss1",
  "http://wellformedweb.org/CommentAPI/":"wfw",                              
  "http://purl.org/rss/1.0/modules/wiki/":"wiki", 
  "http://www.w3.org/XML/1998/namespace":"xml"
}



var gAllowedXHTMLNamespaces = {
  "http://www.w3.org/XML/1998/namespace":"xml",
  "http://www.w3.org/TR/xhtml2":"xhtml2",
  "http://www.w3.org/2005/07/aaa":"aaa",
  
  
  "http://www.w3.org/1999/xhtml":"xhtml"
}

function FeedResult() {}
FeedResult.prototype = {
  bozo: false,
  doc: null,
  version: null,
  headers: null,
  uri: null,
  stylesheet: null,

  registerExtensionPrefix: function FR_registerExtensionPrefix(ns, prefix) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  QueryInterface: function FR_QI(iid) {
    if (iid.equals(Ci.nsIFeedResult) ||
        iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NOINTERFACE;
  },
}  

function Feed() {
  this.subtitle = null;
  this.title = null;
  this.items = Cc[ARRAY_CONTRACTID].createInstance(Ci.nsIMutableArray);
  this.link = null;
  this.id = null;
  this.generator = null;
  this.authors = Cc[ARRAY_CONTRACTID].createInstance(Ci.nsIMutableArray);
  this.contributors = Cc[ARRAY_CONTRACTID].createInstance(Ci.nsIMutableArray);
  this.baseURI = null;
}

Feed.prototype = {
  searchLists: {
    title: ["title", "rss1:title", "atom03:title", "atom:title"],
    subtitle: ["description","dc:description","rss1:description",
               "atom03:tagline","atom:subtitle"],
    items: ["items","atom03_entries","entries"],
    id: ["atom:id","rdf:about"],
    generator: ["generator"],
    authors : ["authors"],
    contributors: ["contributors"],
    title: ["title","rss1:title", "atom03:title","atom:title"],
    link:  [["link",strToURI],["rss1:link",strToURI]],
    categories: ["categories", "dc:subject"],
    rights: ["atom03:rights","atom:rights"],
    cloud: ["cloud"],
    image: ["image", "rss1:image", "atom:logo"],
    textInput: ["textInput", "rss1:textinput"],
    skipDays: ["skipDays"],
    skipHours: ["skipHours"],
    updated: ["pubDate", "lastBuildDate", "atom03:modified", "dc:date",
              "dcterms:modified", "atom:updated"]
  },

  normalize: function Feed_normalize() {
    fieldsToObj(this, this.searchLists);
    if (this.skipDays)
      this.skipDays = this.skipDays.getProperty("days");
    if (this.skipHours)
      this.skipHours = this.skipHours.getProperty("hours");

    if (this.updated)
      this.updated = dateParse(this.updated);

    
    if (bagHasKey(this.fields, "links"))
      this._atomLinksToURI();

    
    if (this.image && bagHasKey(this.image, "url"))
      this._resolveImageLink();

    this._resetBagMembersToRawText([this.searchLists.subtitle, 
                                    this.searchLists.title]);
  },

  _atomLinksToURI: function Feed_linkToURI() {
    var links = this.fields.getPropertyAsInterface("links", Ci.nsIArray);
    var alternates = findAtomLinks("alternate", links);
    if (alternates.length > 0) {
      var href = alternates[0].getPropertyAsAString("href");
      var base;
      if (bagHasKey(alternates[0], "xml:base"))
        base = alternates[0].getPropertyAsAString("xml:base");
      this.link = this._resolveURI(href, base);
    }
  },

  _resolveImageLink: function Feed_resolveImageLink() {
    var base;
    if (bagHasKey(this.image, "xml:base"))
      base = this.image.getPropertyAsAString("xml:base");
    var url = this._resolveURI(this.image.getPropertyAsAString("url"), base);
    if (url)
      this.image.setPropertyAsAString("url", url.spec);
  },

  _resolveURI: function Feed_resolveURI(linkSpec, baseSpec) {
    var uri = null;
    try {
      var base = baseSpec ? strToURI(baseSpec, this.baseURI) : this.baseURI;
      uri = strToURI(linkSpec, base);
    }
    catch(e) {
      LOG(e);
    }

    return uri;
  },

  
  _resetBagMembersToRawText: function Feed_resetBagMembers(fieldLists) {
    for (var i=0; i<fieldLists.length; i++) {      
      for (var j=0; j<fieldLists[i].length; j++) {
        if (bagHasKey(this.fields, fieldLists[i][j])) {
          var textConstruct = this.fields.getProperty(fieldLists[i][j]);
          this.fields.setPropertyAsAString(fieldLists[i][j],
                                           textConstruct.text);
        }
      }
    }
  },
   
  QueryInterface: function Feed_QI(iid) {
    if (iid.equals(Ci.nsIFeed) ||
        iid.equals(Ci.nsIFeedContainer) ||
        iid.equals(Ci.nsISupports))
    return this;
    throw Cr.NS_ERROR_NOINTERFACE;
  }
}

function Entry() {
  this.summary = null;
  this.content = null;
  this.title = null;
  this.fields = Cc["@mozilla.org/hash-property-bag;1"].
    createInstance(Ci.nsIWritablePropertyBag2);
  this.link = null;
  this.id = null;
  this.baseURI = null;
  this.updated = null;
  this.published = null;
  this.authors = Cc[ARRAY_CONTRACTID].createInstance(Ci.nsIMutableArray);
  this.contributors = Cc[ARRAY_CONTRACTID].createInstance(Ci.nsIMutableArray);
}
  
Entry.prototype = {
  fields: null,
  enclosures: null,
  mediaContent: null,
  
  searchLists: {
    title: ["title", "rss1:title", "atom03:title", "atom:title"],
    link: [["link",strToURI],["rss1:link",strToURI]],
    id: [["guid", makePropGetter("guid")], "rdf:about",
         "atom03:id", "atom:id"],
    authors : ["authors"],
    contributors: ["contributors"],
    summary: ["description", "rss1:description", "dc:description",
              "atom03:summary", "atom:summary"],
    content: ["content:encoded","atom03:content","atom:content"],
    rights: ["atom03:rights","atom:rights"],
    published: ["atom03:issued", "dcterms:issued", "atom:published"],
    updated: ["pubDate", "atom03:modified", "dc:date", "dcterms:modified",
              "atom:updated"]
  },
  
  normalize: function Entry_normalize() {
    fieldsToObj(this, this.searchLists);
 
    
    if (bagHasKey(this.fields, "links"))
      this._atomLinksToURI();
 
    
    if (!this.link && bagHasKey(this.fields, "guid")) {
      var guid = this.fields.getProperty("guid");
      var isPermaLink = true;
      
      if (bagHasKey(guid, "isPermaLink"))
        isPermaLink = new Boolean(guid.getProperty("isPermaLink"));
      
      if (guid && isPermaLink)
        this.link = strToURI(guid.getProperty("guid"));
    }

    if (this.updated)
      this.updated = dateParse(this.updated);
    if (this.published)
      this.published = dateParse(this.published);

    this._resetBagMembersToRawText([this.searchLists.content, 
                                    this.searchLists.summary, 
                                    this.searchLists.title]);
  },
  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFeedEntry) ||
        iid.equals(Ci.nsIFeedContainer) ||
        iid.equals(Ci.nsISupports))
    return this;

    throw Cr.NS_ERROR_NOINTERFACE;
  }
}

Entry.prototype._atomLinksToURI = Feed.prototype._atomLinksToURI;
Entry.prototype._resolveURI = Feed.prototype._resolveURI;
Entry.prototype._resetBagMembersToRawText = 
   Feed.prototype._resetBagMembersToRawText;


function TextConstruct() {
  this.lang = null;
  this.base = null;
  this.type = "text";
  this.text = null;
}

TextConstruct.prototype = {
  plainText: function TC_plainText() {
    if (this.type != "text") {
      return gUnescapeHTML.unescape(stripTags(this.text));
    }
    return this.text;
  },

  createDocumentFragment: function TC_createDocumentFragment(element) {
    if (this.type == "text") {
      var doc = element.ownerDocument;
      var docFragment = doc.createDocumentFragment();
      var node = doc.createTextNode(this.text);
      docFragment.appendChild(node);
      return docFragment;
    }
    var isXML;
    if (this.type == "xhtml")
      isXML = true
    else if (this.type == "html")
      isXML = false;
    else
      return null;

    return gUnescapeHTML.parseFragment(this.text, isXML, this.base, element);
  },
 
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFeedTextConstruct) ||
        iid.equals(Ci.nsISupports))
    return this;

    throw Cr.NS_ERROR_NOINTERFACE;
  }
}


function Generator() {
  this.lang = null;
  this.agent = null;
  this.version = null;
  this.uri = null;

  
  this._attributes = null;
  this.baseURI = null;
}

Generator.prototype = {

  get attributes() {
    return this._attributes;
  },

  set attributes(value) {
    this._attributes = value;
    this.version = this._attributes.getValueFromName("","version");
    var uriAttribute = this._attributes.getValueFromName("","uri") ||
                       this._attributes.getValueFromName("","url");
    this.uri = strToURI(uriAttribute, this.baseURI);

    
    uriAttribute = this._attributes.getValueFromName(RDF_NS,"resource");
    if (uriAttribute) {
      this.agent = uriAttribute;
      this.uri = strToURI(uriAttribute, this.baseURI);
    }
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFeedGenerator) ||
        iid.equals(Ci.nsIFeedElementBase) ||
        iid.equals(Ci.nsISupports))
    return this;

    throw Cr.NS_ERROR_NOINTERFACE;
  }
}

function Person() {
  this.name = null;
  this.uri = null;
  this.email = null;

  
  this.attributes = null;
  this.baseURI = null;
}

Person.prototype = {
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFeedPerson) ||
        iid.equals(Ci.nsIFeedElementBase) ||
        iid.equals(Ci.nsISupports))
    return this;

    throw Cr.NS_ERROR_NOINTERFACE;
  }
}









function fieldsToObj(container, fields) {
  var props,prop,field,searchList;
  for (var key in fields) {
    searchList = fields[key];
    for (var i=0; i < searchList.length; ++i) {
      props = searchList[i];
      prop = null;
      field = isArray(props) ? props[0] : props;
      try {
        prop = container.fields.getProperty(field);
      } 
      catch(e) { 
      }
      if (prop) {
        prop = isArray(props) ? props[1](prop) : prop;
        container[key] = prop;
      }
    }
  }
}







function LC(element) {
  return element.localName.toLowerCase();
}



function atomGenerator(s, generator) {
  generator.QueryInterface(Ci.nsIFeedGenerator);
  generator.agent = trimString(s);
  return generator;
}


function atomLogo(s, logo) {
  logo.setPropertyAsAString("url", trimString(s));
}


function rssCatTerm(s, cat) {
  
  cat.setPropertyAsAString("term", trimString(s));
  return cat;
} 


function rssGuid(s, guid) {
  guid.setPropertyAsAString("guid", trimString(s));
  return guid;
}














function rssAuthor(s,author) {
  author.QueryInterface(Ci.nsIFeedPerson);
  
  var chars = trimString(s);
  var matches = chars.match(/(.*)\((.*)\)/);
  var emailCheck = 
    /^([a-zA-Z0-9_\.\-])+\@(([a-zA-Z0-9\-])+\.)+([a-zA-Z0-9]{2,4})+$/;
  if (matches) {
    var match1 = trimString(matches[1]);
    var match2 = trimString(matches[2]);
    if (match2.indexOf("mailto:") == 0)
      match2 = match2.substring(7);
    if (emailCheck.test(match1)) {
      author.email = match1;
      author.name = match2;
    }
    else if (emailCheck.test(match2)) {
      author.email = match2;
      author.name = match1;
    }
    else {
      
      author.name = match1 + " (" + match2 + ")";
    }
  }
  else {
    author.name = chars;
    if (chars.indexOf('@'))
      author.email = chars;
  }
  return author;
}





function rssArrayElement(s) {
  var str = Cc["@mozilla.org/supports-string;1"].
              createInstance(Ci.nsISupportsString);
  str.data = s;
  str.QueryInterface(Ci.nsISupportsString);
  return str;
}









function isValidRFC822Date(aDateStr) {
  var regex = new RegExp(RFC822_RE);
  return regex.test(aDateStr);
}







function trimString(s) {
  return(s.replace(/^\s+/, "").replace(/\s+$/, ""));
}


const RFC822_RE = "^((Mon|Tue|Wed|Thu|Fri|Sat|Sun)([a-z]+)?,? *)?\\d\\d?"
+ " +(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)([a-z]+)?"
+ " +\\d\\d(\\d\\d)? +\\d?\\d:\\d\\d(:\\d\\d)?"
+ " +([+-]?\\d\\d\\d\\d|GMT|UT|(E|C|M|P)(ST|DT)|[A-IK-Z])$";










function dateParse(dateString) {
  var date = trimString(dateString);

  if (date.search(/^\d\d\d\d/) != -1) 
    return W3CToIETFDate(dateString);

  if (isValidRFC822Date(date))
    return date; 
  
  if (!isNaN(parseInt(date, 10))) { 
    
    var d = new Date(parseInt(date, 10) * 1000);
    var now = new Date();
    var yeardiff = now.getFullYear() - d.getFullYear();
    if ((yeardiff >= 0) && (yeardiff < 3)) {
      
      
      
      return d.toString();
    }
  }
  
  return null;
} 


const XHTML_NS = "http://www.w3.org/1999/xhtml";


function XHTMLHandler(processor, isAtom, waiPrefixes) {
  this._buf = "";
  this._processor = processor;
  this._depth = 0;
  this._isAtom = isAtom;
  
  this._inScopeNS = [];
  this._waiPrefixes = waiPrefixes;
}



XHTMLHandler.prototype = {

   
   
  _isInScope: function XH__isInScope(ns) {
    for (var i in this._inScopeNS) {
      for (var uri in this._inScopeNS[i]) {
        if (this._inScopeNS[i][uri] == ns)
          return true;
      }
    }
    return false;
  },

  startDocument: function XH_startDocument() {
  },
  endDocument: function XH_endDocument() {
  },
  startElement: function XH_startElement(uri, localName, qName, attributes) {
    ++this._depth;
    this._inScopeNS.push([]);

    
    
    
    if (this._isAtom && this._depth == 1 && localName == "div")
      return;

    
    if (uri == XHTML_NS) {
      this._buf += "<" + localName;
      var uri;
      for (var i=0; i < attributes.length; ++i) {
        uri = attributes.getURI(i);
        
        if (uri == "") { 
          this._buf += (" " + attributes.getLocalName(i) + "='" +
                        xmlEscape(attributes.getValue(i)) + "'");
        } else {
          
          var prefix = gAllowedXHTMLNamespaces[uri];
          if (prefix != null) {
            
            var attributeValue = xmlEscape(attributes.getValue(i));

            
            var rolePrefix = "";
            if (attributes.getLocalName(i) == "role") {
              for (var aPrefix in this._waiPrefixes) {
                if (attributeValue.indexOf(aPrefix + ":") == 0) {     
                  
                  
                  
                  
                  var isCollision = false;
                  for (var uriKey in gAllowedXHTMLNamespaces) {
                    if (gAllowedXHTMLNamespaces[uriKey] == aPrefix)
                      isCollision = true;
                  }
                  
                  if (isCollision) {
                    rolePrefix = aPrefix + i;
                    attributeValue = 
                      rolePrefix + ":" + 
                      attributeValue.substring(aPrefix.length + 1);
                  } else {
                    rolePrefix = aPrefix;
                  }

                  break;
                }
              }

              if (rolePrefix)
                this._buf += (" xmlns:" + rolePrefix + 
                              "='" + WAIROLE_NS + "'");
            }

            
            
            this._buf += (" " + prefix + ":" + 
                          attributes.getLocalName(i) + 
                          "='" + attributeValue + "'");

            
            if (prefix != "xml" && !this._isInScope(uri)) {
              this._inScopeNS[this._inScopeNS.length - 1].push(uri);
              this._buf += " xmlns:" + prefix + "='" + uri + "'";
            }
          }
        }
      }
      this._buf += ">";
    }
  },
  endElement: function XH_endElement(uri, localName, qName) {
    --this._depth;
    this._inScopeNS.pop();

    
    if (this._isAtom && this._depth == 0 && localName == "div")
      return;

    
    if (this._depth < 0) {
      this._processor.returnFromXHTMLHandler(trimString(this._buf),
                                             uri, localName, qName);
      return;
    }
    
    if (uri == XHTML_NS) {
      this._buf += "</" + localName + ">";
    }
  },
  characters: function XH_characters(data) {
    this._buf += xmlEscape(data);
  },
  startPrefixMapping: function XH_startPrefixMapping(prefix, uri) {
    if (prefix && uri == WAIROLE_NS) 
      this._waiPrefixes[prefix] = WAIROLE_NS;
  },
  endPrefixMapping: function FP_endPrefixMapping(prefix) {
    if (prefix)
      delete this._waiPrefixes[prefix];
  },
  processingInstruction: function XH_processingInstruction() {
  }, 
}





function ExtensionHandler(processor) {
  this._buf = "";
  this._depth = 0;
  this._hasChildElements = false;

  
  this._processor = processor;

  
  this._localName = null;
  this._uri = null;
  this._qName = null;
  this._attrs = null;
}

ExtensionHandler.prototype = {
  startDocument: function EH_startDocument() {
  },
  endDocument: function EH_endDocument() {
  },
  startElement: function EH_startElement(uri, localName, qName, attrs) {
    ++this._depth;
    var prefix = gNamespaces[uri] ? gNamespaces[uri] + ":" : "";
    var key =  prefix + localName;
    
    if (this._depth == 1) {
      this._uri = uri;
      this._localName = localName;
      this._qName = qName;
      this._attrs = attrs;
    }
    
    
    this._hasChildElements = (this._depth > 1);
    
  },
  endElement: function EH_endElement(uri, localName, qName) {
    --this._depth;
    if (this._depth == 0) {
      var text = this._hasChildElements ? null : trimString(this._buf);
      this._processor.returnFromExtHandler(this._uri, this._localName, 
                                           text, this._attrs);
    }
  },
  characters: function EH_characters(data) {
    if (!this._hasChildElements)
      this._buf += data;
  },
  startPrefixMapping: function EH_startPrefixMapping() {
  },
  endPrefixMapping: function EH_endPrefixMapping() {
  },
  processingInstruction: function EH_processingInstruction() {
  }, 
};







 
function ElementInfo(fieldName, containerClass, closeFunc, isArray) {
  this.fieldName = fieldName;
  this.containerClass = containerClass;
  this.closeFunc = closeFunc;
  this.isArray = isArray;
  this.isWrapper = false;
}




function FeedElementInfo(fieldName, feedVersion) {
  this.isWrapper = false;
  this.fieldName = fieldName;
  this.feedVersion = feedVersion;
}






function WrapperElementInfo(fieldName) {
  this.isWrapper = true;
  this.fieldName = fieldName;
}


function FeedProcessor() {
  this._reader = Cc[SAX_CONTRACTID].createInstance(Ci.nsISAXXMLReader);
  this._buf =  "";
  this._feed = Cc[BAG_CONTRACTID].createInstance(Ci.nsIWritablePropertyBag2);
  this._handlerStack = [];
  this._xmlBaseStack = []; 
  this._depth = 0;
  this._state = "START";
  this._result = null;
  this._extensionHandler = null;
  this._xhtmlHandler = null;
  
  
  this._waiPrefixes = {};

  
  this.listener = null;

  
  
  this._textConstructs = {"atom:title":"text",
                          "atom:summary":"text",
                          "atom:rights":"text",
                          "atom:content":"text",
                          "atom:subtitle":"text",
                          "description":"html",
                          "rss1:description":"html",
                          "dc:description":"html",
                          "content:encoded":"html",
                          "title":"text",
                          "rss1:title":"text",
                          "atom03:title":"text",
                          "atom03:tagline":"text",
                          "atom03:summary":"text",
                          "atom03:content":"text"};
  this._stack = [];

  this._trans = {   
    "START": {
      
      "rss": new FeedElementInfo("RSS2", "rss2"),

      
      
      "rdf:RDF": new WrapperElementInfo("RDF"),

      
      "atom:feed": new FeedElementInfo("Atom", "atom"),

      
      "atom03:feed": new FeedElementInfo("Atom03", "atom03"),
    },
    
    
    "IN_RSS2": {
      "channel": new WrapperElementInfo("channel")
    },

    "IN_CHANNEL": {
      "item": new ElementInfo("items", Cc[ENTRY_CONTRACTID], null, true),
      "managingEditor": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                        rssAuthor, true),
      "dc:creator": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                    rssAuthor, true),
      "dc:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                   rssAuthor, true),
      "dc:contributor": new ElementInfo("contributors", Cc[PERSON_CONTRACTID],
                                         rssAuthor, true),
      "category": new ElementInfo("categories", null, rssCatTerm, true),
      "cloud": new ElementInfo("cloud", null, null, false),
      "image": new ElementInfo("image", null, null, false),
      "textInput": new ElementInfo("textInput", null, null, false),
      "skipDays": new ElementInfo("skipDays", null, null, false),
      "skipHours": new ElementInfo("skipHours", null, null, false),
      "generator": new ElementInfo("generator", Cc[GENERATOR_CONTRACTID],
                                   atomGenerator, false),
    },

    "IN_ITEMS": {
      "author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                rssAuthor, true),
      "dc:creator": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                    rssAuthor, true),
      "dc:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                   rssAuthor, true),
      "dc:contributor": new ElementInfo("contributors", Cc[PERSON_CONTRACTID],
                                         rssAuthor, true),
      "category": new ElementInfo("categories", null, rssCatTerm, true),
      "enclosure": new ElementInfo("enclosure", null, null, true),
      "guid": new ElementInfo("guid", null, rssGuid, false)
    },

    "IN_SKIPDAYS": {
      "day": new ElementInfo("days", null, rssArrayElement, true)
    },

    "IN_SKIPHOURS":{
      "hour": new ElementInfo("hours", null, rssArrayElement, true)
    },

    
    "IN_RDF": {
      
      "rss1:channel": new FeedElementInfo("rdf_channel", "rss1"),
      "rss1:image": new ElementInfo("image", null, null, false),
      "rss1:textinput": new ElementInfo("textInput", null, null, false),
      "rss1:item": new ElementInfo("items", Cc[ENTRY_CONTRACTID], null, true),
    },

    "IN_RDF_CHANNEL": {
      "admin:generatorAgent": new ElementInfo("generator",
                                              Cc[GENERATOR_CONTRACTID],
                                              null, false),
      "dc:creator": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                    rssAuthor, true),
      "dc:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                   rssAuthor, true),
      "dc:contributor": new ElementInfo("contributors", Cc[PERSON_CONTRACTID],
                                         rssAuthor, true),
    },

    
    "IN_ATOM": {
      "atom:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                     null, true),
      "atom:generator": new ElementInfo("generator", Cc[GENERATOR_CONTRACTID],
                                        atomGenerator, false),
      "atom:contributor": new ElementInfo("contributors",  Cc[PERSON_CONTRACTID],
                                          null, true),
      "atom:link": new ElementInfo("links", null, null, true),
      "atom:logo": new ElementInfo("atom:logo", null, atomLogo, false),
      "atom:entry": new ElementInfo("entries", Cc[ENTRY_CONTRACTID],
                                    null, true)
    },

    "IN_ENTRIES": {
      "atom:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                     null, true),
      "atom:contributor": new ElementInfo("contributors", Cc[PERSON_CONTRACTID],
                                          null, true),
      "atom:link": new ElementInfo("links", null, null, true),
    },

    
    "IN_ATOM03": {
      "atom03:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                       null, true),
      "atom03:contributor": new ElementInfo("contributors",
                                            Cc[PERSON_CONTRACTID],
                                            null, true),
      "atom03:link": new ElementInfo("links", null, null, true),
      "atom03:entry": new ElementInfo("atom03_entries", Cc[ENTRY_CONTRACTID],
                                      null, true),
      "atom03:generator": new ElementInfo("generator", Cc[GENERATOR_CONTRACTID],
                                          atomGenerator, false),
    },

    "IN_ATOM03_ENTRIES": {
      "atom03:author": new ElementInfo("authors", Cc[PERSON_CONTRACTID],
                                       null, true),
      "atom03:contributor": new ElementInfo("contributors",
                                            Cc[PERSON_CONTRACTID],
                                            null, true),
      "atom03:link": new ElementInfo("links", null, null, true),
      "atom03:entry": new ElementInfo("atom03_entries", Cc[ENTRY_CONTRACTID],
                                      null, true)
    }
  }
}


FeedProcessor.prototype = { 
  
  
  _init: function FP_init(uri) {
    this._reader.contentHandler = this;
    this._reader.errorHandler = this;
    this._result = Cc[FR_CONTRACTID].createInstance(Ci.nsIFeedResult);
    if (uri) {
      this._result.uri = uri;
      this._reader.baseURI = uri;
      this._xmlBaseStack[0] = uri;
    }
  },

  
  
  
  _docVerified: function FP_docVerified(version) {
    this._result.doc = Cc[FEED_CONTRACTID].createInstance(Ci.nsIFeed);
    this._result.doc.baseURI = 
      this._xmlBaseStack[this._xmlBaseStack.length - 1];
    this._result.doc.fields = this._feed;
    this._result.version = version;
  },

  
  
  _sendResult: function FP_sendResult() {
    try {
      
      if (this._result.doc)
        this._result.doc.normalize();
    }
    catch (e) {
      LOG("FIXME: " + e);
    }

    try {
      if (this.listener != null)
        this.listener.handleResult(this._result);
    }
    finally {
      this._result = null;
    }
  },

  
  parseFromStream: function FP_parseFromStream(stream, uri) {
    this._init(uri);
    this._reader.parseFromStream(stream, null, stream.available(), 
                                 "application/xml");
    this._reader = null;
  },

  parseFromString: function FP_parseFromString(inputString, uri) {
    this._init(uri);
    this._reader.parseFromString(inputString, "application/xml");
    this._reader = null;
  },

  parseAsync: function FP_parseAsync(requestObserver, uri) {
    this._init(uri);
    this._reader.parseAsync(requestObserver);
  },

  

  
  
  onStartRequest: function FP_onStartRequest(request, context) {
    this._reader.onStartRequest(request, context);
  },

  onStopRequest: function FP_onStopRequest(request, context, statusCode) {
    try {
      this._reader.onStopRequest(request, context, statusCode);
    }
    finally {
      this._reader = null;
    }
  },

  onDataAvailable:
  function FP_onDataAvailable(request, context, inputStream, offset, count) {
    this._reader.onDataAvailable(request, context, inputStream, offset, count);
  },

  

  
  
  
  
  
  fatalError: function FP_reportError() {
    this._result.bozo = true;
    
    this._sendResult();
  },

  

  startDocument: function FP_startDocument() {
    
  },

  endDocument: function FP_endDocument() {
    this._sendResult();
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  startElement: function FP_startElement(uri, localName, qName, attributes) {
    this._buf = "";
    ++this._depth;
    var elementInfo;

    

    
    var base = attributes.getValueFromName(XMLNS, "base");
    if (base) {
      this._xmlBaseStack[this._depth] =
        strToURI(base, this._xmlBaseStack[this._xmlBaseStack.length - 1]);
    }

    
    
    
    
    
    
    var key =  this._prefixForNS(uri) + localName;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if ((this._result.version == "atom" || this._result.version == "atom03") &&
        this._textConstructs[key] != null) {
      var type = attributes.getValueFromName("","type");
      if (type != null && type.indexOf("xhtml") >= 0) {
        this._xhtmlHandler = 
          new XHTMLHandler(this, (this._result.version == "atom"), 
                           this._waiPrefixes);
        this._reader.contentHandler = this._xhtmlHandler;
        return;
      }
    }

    
    
    
    if (this._trans[this._state] && this._trans[this._state][key]) {
      elementInfo = this._trans[this._state][key];
    }
    else {
      
      this._extensionHandler = new ExtensionHandler(this);
      this._reader.contentHandler = this._extensionHandler;
      this._extensionHandler.startElement(uri, localName, qName, attributes);
      return;
    }
      
    
    
    this._handlerStack[this._depth] = elementInfo; 
    if (elementInfo.isWrapper) {
      this._state = "IN_" + elementInfo.fieldName.toUpperCase();
      this._stack.push([this._feed, this._state]);
    } 
    else if (elementInfo.feedVersion) {
      this._state = "IN_" + elementInfo.fieldName.toUpperCase();

      
      if (elementInfo.feedVersion == "rss2")
        elementInfo.feedVersion = this._findRSSVersion(attributes);
      else if (uri == RSS090NS)
        elementInfo.feedVersion = "rss090";

      this._docVerified(elementInfo.feedVersion);
      this._stack.push([this._feed, this._state]);
      this._mapAttributes(this._feed, attributes);
    }
    else {
      this._state = this._processComplexElement(elementInfo, attributes);
    }
  },

  
  
  
  
  
  endElement:  function FP_endElement(uri, localName, qName) {
    var elementInfo = this._handlerStack[this._depth];
    
    if (elementInfo && !elementInfo.isWrapper)
      this._closeComplexElement(elementInfo);
  
    
    if (this._xmlBaseStack.length == this._depth + 1)
      this._xmlBaseStack = this._xmlBaseStack.slice(0, this._depth);

    
    if (this._stack.length > 0)
      this._state = this._stack[this._stack.length - 1][1];
    this._handlerStack = this._handlerStack.slice(0, this._depth);
    --this._depth;
  },

  
  
  characters: function FP_characters(data) {
    this._buf += data;
  },
  
  
  
  startPrefixMapping: function FP_startPrefixMapping(prefix, uri) {
    
    
    
    if (prefix && uri == WAIROLE_NS) 
      this._waiPrefixes[prefix] = WAIROLE_NS;
  },
  
  endPrefixMapping: function FP_endPrefixMapping(prefix) {
    if (prefix)
      delete this._waiPrefixes[prefix];
  },
  
  processingInstruction: function FP_processingInstruction(target, data) {
    if (target == "xml-stylesheet") {
      var hrefAttribute = data.match(/href=[\"\'](.*?)[\"\']/);
      if (hrefAttribute && hrefAttribute.length == 2) 
        this._result.stylesheet = gIoService.newURI(hrefAttribute[1], null,
                                                    this._result.uri);
    }
  },

  

  
  
  _processComplexElement:
  function FP__processComplexElement(elementInfo, attributes) {
    var obj, key, prefix;

    
    
    if (elementInfo.containerClass == Cc[ENTRY_CONTRACTID]) {
      obj = elementInfo.containerClass.createInstance(Ci.nsIFeedEntry);
      obj.baseURI = this._xmlBaseStack[this._xmlBaseStack.length - 1];
      this._mapAttributes(obj.fields, attributes);
    }
    else if (elementInfo.containerClass) {
      obj = elementInfo.containerClass.createInstance(Ci.nsIFeedElementBase);
      obj.baseURI = this._xmlBaseStack[this._xmlBaseStack.length - 1];
      obj.attributes = attributes; 
    }
    else {
      obj = Cc[BAG_CONTRACTID].createInstance(Ci.nsIWritablePropertyBag2);
      this._mapAttributes(obj, attributes);
    }

    
    
    
    var newProp;

    
    var container = this._stack[this._stack.length - 1][0];

    
    var prop;
    try {
      prop = container.getProperty(elementInfo.fieldName);
    }
    catch(e) {
    }
    
    if (elementInfo.isArray) {
      if (!prop) {
        container.setPropertyAsInterface(elementInfo.fieldName,
                                         Cc[ARRAY_CONTRACTID].
                                         createInstance(Ci.nsIMutableArray));
      }

      newProp = container.getProperty(elementInfo.fieldName);
      
      
      
      newProp.QueryInterface(Ci.nsIMutableArray);
      newProp.appendElement(obj,false);
      
      
      
      if (isIFeedContainer(obj))
        newProp = obj.fields; 

    }
    else {
      
      if (!prop) {
        container.setPropertyAsInterface(elementInfo.fieldName,obj);
      }
      newProp = container.getProperty(elementInfo.fieldName);
    }
    
    
    var newState = "IN_" + elementInfo.fieldName.toUpperCase();
    this._stack.push([newProp, newState, obj]);
    return newState;
  },

  
  
  
  
  _closeComplexElement: function FP__closeComplexElement(elementInfo) {
    var stateTuple = this._stack.pop();
    var container = stateTuple[0];
    var containerParent = stateTuple[2];
    var element = null;
    var isArray = isIArray(container);

    
    
    if (isArray)
      element = container.queryElementAt(container.length - 1, Ci.nsISupports);
    else
      element = container;

    
    if (elementInfo.closeFunc)
      element = elementInfo.closeFunc(this._buf, element);

    
    
    if (elementInfo.containerClass == Cc[ENTRY_CONTRACTID])
      containerParent.normalize();

    
    if (isArray)
      container.replaceElementAt(element, container.length - 1, false);
  },
  
  _prefixForNS: function FP_prefixForNS(uri) {
    if (!uri)
      return "";
    var prefix = gNamespaces[uri];
    if (prefix)
      return prefix + ":";
    if (uri.toLowerCase().indexOf("http://backend.userland.com") == 0)
      return "";
    else
      return null;
  },

  _mapAttributes: function FP__mapAttributes(bag, attributes) {
    
    
    for (var i = 0; i < attributes.length; ++i) {
      var key = this._prefixForNS(attributes.getURI(i)) + attributes.getLocalName(i);
      var val = attributes.getValue(i);
      bag.setPropertyAsAString(key, val);
    }
  },

  
  _findRSSVersion: function FP__findRSSVersion(attributes) {
    var versionAttr = trimString(attributes.getValueFromName("", "version"));
    var versions = { "0.91":"rss091",
                     "0.92":"rss092",
                     "0.93":"rss093",
                     "0.94":"rss094" }
    if (versions[versionAttr])
      return versions[versionAttr];
    if (versionAttr.substr(0,2) != "2.")
      return "rssUnknown";
    return "rss2";
  },

  
  
  returnFromExtHandler:
  function FP_returnExt(uri, localName, chars, attributes) {
    --this._depth;

    
    this._reader.contentHandler = this;
    if (localName == null && chars == null)
      return;

    
    if (this._state == "IN_RDF")
      return;
    
    
    var top = this._stack[this._stack.length - 1];
    if (!top) 
      return;

    var container = top[0];
    
    if (isIArray(container)) {
      var contract = this._handlerStack[this._depth].containerClass;
      
      if (contract && contract != Cc[ENTRY_CONTRACTID]) {
        var el = container.queryElementAt(container.length - 1, 
                                          Ci.nsIFeedElementBase);
        
        if (contract == Cc[PERSON_CONTRACTID]) 
          el.QueryInterface(Ci.nsIFeedPerson);
        else
          return; 

        var propName = localName;
        var prefix = gNamespaces[uri];

        
        if ((uri == "" || 
             prefix &&
             ((prefix.indexOf("atom") > -1) ||
              (prefix.indexOf("rss") > -1))) && 
            (propName == "url" || propName == "href"))
          propName = "uri";
        
        try {
          if (el[propName] !== "undefined") {
            var propValue = chars;
            
            if (propName == "uri") {
              var base = this._xmlBaseStack[this._xmlBaseStack.length - 1];
              propValue = strToURI(chars, base);
            }
            el[propName] = propValue;
          }
        }
        catch(e) {
          
        }
        
        return; 
      } 
      else {
        container = container.queryElementAt(container.length - 1, 
                                             Ci.nsIWritablePropertyBag2);
      }
    }
    
    
    var propName = this._prefixForNS(uri) + localName;

    
    
    if (this._textConstructs[propName] != null &&
        this._handlerStack[this._depth].containerClass !== null) {
      var newProp = Cc[TEXTCONSTRUCT_CONTRACTID].
                    createInstance(Ci.nsIFeedTextConstruct);
      newProp.text = chars;
      
      var type = this._textConstructs[propName];
      var typeAttribute = attributes.getValueFromName("","type");
      if (this._result.version == "atom" && typeAttribute != null) {
        type = typeAttribute;
      }
      else if (this._result.version == "atom03" && typeAttribute != null) {
        if (typeAttribute.toLowerCase().indexOf("xhtml") >= 0) {
          type = "xhtml";
        }
        else if (typeAttribute.toLowerCase().indexOf("html") >= 0) {
          type = "html";
        }
        else if (typeAttribute.toLowerCase().indexOf("text") >= 0) {
          type = "text";
        }
      }
      
      
      if (this._result.version.indexOf("rss") >= 0 &&
          this._handlerStack[this._depth].containerClass != ENTRY_CONTRACTID) {
        type = "text";
      }
      newProp.type = type;
      newProp.base = this._xmlBaseStack[this._xmlBaseStack.length - 1];
      container.setPropertyAsInterface(propName, newProp);
    }
    else {
      container.setPropertyAsAString(propName, chars);
    }
  },

  
  
  
  
  returnFromXHTMLHandler:
  function FP_returnFromXHTMLHandler(chars, uri, localName, qName) {
    
    this._reader.contentHandler = this;

    
    var top = this._stack[this._stack.length - 1];
    if (!top) 
      return;
    var container = top[0];

    
    var newProp =  newProp = Cc[TEXTCONSTRUCT_CONTRACTID].
                   createInstance(Ci.nsIFeedTextConstruct);
    newProp.text = chars;
    newProp.type = "xhtml";
    newProp.base = this._xmlBaseStack[this._xmlBaseStack.length - 1];
    container.setPropertyAsInterface(this._prefixForNS(uri) + localName,
                                     newProp);
    
    
    
    
    
    this.endElement(uri, localName, qName);
  },

  
  QueryInterface: function FP_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFeedProcessor) ||
        iid.equals(Ci.nsISAXContentHandler) ||
        iid.equals(Ci.nsISAXErrorHandler) ||
        iid.equals(Ci.nsIStreamListener) ||
        iid.equals(Ci.nsIRequestObserver) ||
        iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NOINTERFACE;
  },
}

const FP_CONTRACTID = "@mozilla.org/feed-processor;1";
const FP_CLASSID = Components.ID("{26acb1f0-28fc-43bc-867a-a46aabc85dd4}");
const FP_CLASSNAME = "Feed Processor";
const FR_CONTRACTID = "@mozilla.org/feed-result;1";
const FR_CLASSID = Components.ID("{072a5c3d-30c6-4f07-b87f-9f63d51403f2}");
const FR_CLASSNAME = "Feed Result";
const FEED_CONTRACTID = "@mozilla.org/feed;1";
const FEED_CLASSID = Components.ID("{5d0cfa97-69dd-4e5e-ac84-f253162e8f9a}");
const FEED_CLASSNAME = "Feed";
const ENTRY_CONTRACTID = "@mozilla.org/feed-entry;1";
const ENTRY_CLASSID = Components.ID("{8e4444ff-8e99-4bdd-aa7f-fb3c1c77319f}");
const ENTRY_CLASSNAME = "Feed Entry";
const TEXTCONSTRUCT_CONTRACTID = "@mozilla.org/feed-textconstruct;1";
const TEXTCONSTRUCT_CLASSID =
  Components.ID("{b992ddcd-3899-4320-9909-924b3e72c922}");
const TEXTCONSTRUCT_CLASSNAME = "Feed Text Construct";
const GENERATOR_CONTRACTID = "@mozilla.org/feed-generator;1";
const GENERATOR_CLASSID =
  Components.ID("{414af362-9ad8-4296-898e-62247f25a20e}");
const GENERATOR_CLASSNAME = "Feed Generator";
const PERSON_CONTRACTID = "@mozilla.org/feed-person;1";
const PERSON_CLASSID = Components.ID("{95c963b7-20b2-11db-92f6-001422106990}");
const PERSON_CLASSNAME = "Feed Person";

function GenericComponentFactory(ctor) {
  this._ctor = ctor;
}

GenericComponentFactory.prototype = {

  _ctor: null,

  
  createInstance: function(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new this._ctor()).QueryInterface(iid);
  },

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

};

var Module = {
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIModule) || 
        iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  getClassObject: function(cm, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    if (cid.equals(FP_CLASSID))
      return new GenericComponentFactory(FeedProcessor);
    if (cid.equals(FR_CLASSID))
      return new GenericComponentFactory(FeedResult);
    if (cid.equals(FEED_CLASSID))
      return new GenericComponentFactory(Feed);
    if (cid.equals(ENTRY_CLASSID))
      return new GenericComponentFactory(Entry);
    if (cid.equals(TEXTCONSTRUCT_CLASSID))
      return new GenericComponentFactory(TextConstruct);
    if (cid.equals(GENERATOR_CLASSID))
      return new GenericComponentFactory(Generator);
    if (cid.equals(PERSON_CLASSID))
      return new GenericComponentFactory(Person);

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  registerSelf: function(cm, file, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    
    cr.registerFactoryLocation(FP_CLASSID, FP_CLASSNAME,
      FP_CONTRACTID, file, location, type);
    
    cr.registerFactoryLocation(FR_CLASSID, FR_CLASSNAME,
      FR_CONTRACTID, file, location, type);
    
    cr.registerFactoryLocation(FEED_CLASSID, FEED_CLASSNAME,
      FEED_CONTRACTID, file, location, type);
    
    cr.registerFactoryLocation(ENTRY_CLASSID, ENTRY_CLASSNAME,
      ENTRY_CONTRACTID, file, location, type);
    
    cr.registerFactoryLocation(TEXTCONSTRUCT_CLASSID, TEXTCONSTRUCT_CLASSNAME,
      TEXTCONSTRUCT_CONTRACTID, file, location, type);
    
    cr.registerFactoryLocation(GENERATOR_CLASSID, GENERATOR_CLASSNAME,
      GENERATOR_CONTRACTID, file, location, type);
    
    cr.registerFactoryLocation(PERSON_CLASSID, PERSON_CLASSNAME,
      PERSON_CONTRACTID, file, location, type);
  },

  unregisterSelf: function(cm, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    
    cr.unregisterFactoryLocation(FP_CLASSID, location);
    
    cr.unregisterFactoryLocation(FR_CLASSID, location);
    
    cr.unregisterFactoryLocation(FEED_CLASSID, location);
    
    cr.unregisterFactoryLocation(ENTRY_CLASSID, location);
    
    cr.unregisterFactoryLocation(TEXTCONSTRUCT_CLASSID, location);
    
    cr.unregisterFactoryLocation(GENERATOR_CLASSID, location);
    
    cr.unregisterFactoryLocation(PERSON_CLASSID, location);
  },

  canUnload: function(cm) {
    return true;
  },
};

function NSGetModule(cm, file) {
  return Module;
}
