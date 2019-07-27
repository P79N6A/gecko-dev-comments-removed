



























var Readability = function(uri, doc) {
  const ENABLE_LOGGING = false;

  this._uri = uri;
  this._doc = doc;
  this._biggestFrame = false;
  this._articleByline = null;
  this._articleDir = null;

  
  this._flags = this.FLAG_STRIP_UNLIKELYS |
                this.FLAG_WEIGHT_CLASSES |
                this.FLAG_CLEAN_CONDITIONALLY;

  
  
  this._parsedPages = {};

  
  
  this._pageETags = {};

  
  this._curPageNum = 1;

  
  if (ENABLE_LOGGING) {
    this.log = function (msg) {
      dump("Reader: (Readability) " + msg);
    };
  } else {
    this.log = function () {};
  }
}

Readability.prototype = {
  FLAG_STRIP_UNLIKELYS: 0x1,
  FLAG_WEIGHT_CLASSES: 0x2,
  FLAG_CLEAN_CONDITIONALLY: 0x4,

  
  
  N_TOP_CANDIDATES: 5,

  
  
  MAX_PAGES: 5,

  
  
  REGEXPS: {
    unlikelyCandidates: /combx|comment|community|disqus|extra|foot|header|menu|remark|rss|shoutbox|sidebar|sponsor|ad-break|agegate|pagination|pager|popup|tweet|twitter/i,
    okMaybeItsACandidate: /and|article|body|column|main|shadow/i,
    positive: /article|body|content|entry|hentry|main|page|pagination|post|text|blog|story/i,
    negative: /hidden|combx|comment|com-|contact|foot|footer|footnote|masthead|media|meta|outbrain|promo|related|scroll|shoutbox|sidebar|sponsor|shopping|tags|tool|widget/i,
    extraneous: /print|archive|comment|discuss|e[\-]?mail|share|reply|all|login|sign|single|utility/i,
    byline: /byline|author|dateline|writtenby/i,
    replaceFonts: /<(\/?)font[^>]*>/gi,
    trim: /^\s+|\s+$/g,
    normalize: /\s{2,}/g,
    videos: /http:\/\/(www\.)?(youtube|vimeo)\.com/i,
    nextLink: /(next|weiter|continue|>([^\|]|$)|»([^\|]|$))/i,
    prevLink: /(prev|earl|old|new|<|«)/i,
    whitespace: /^\s*$/
  },

  DIV_TO_P_ELEMS: [ "A", "BLOCKQUOTE", "DL", "DIV", "IMG", "OL", "P", "PRE", "TABLE", "UL", "SELECT" ],

  





  _postProcessContent: function(articleContent) {
    
    this._fixRelativeUris(articleContent);
  },

  





  _fixRelativeUris: function(articleContent) {
    var scheme = this._uri.scheme;
    var prePath = this._uri.prePath;
    var pathBase = this._uri.pathBase;

    function toAbsoluteURI(uri) {
      
      if (/^[a-zA-Z][a-zA-Z0-9\+\-\.]*:/.test(uri))
        return uri;

      
      if (uri.substr(0, 2) == "//")
        return scheme + "://" + uri.substr(2);

      
      if (uri[0] == "/")
        return prePath + uri;

      
      
      return pathBase + uri;
    }

    function convertRelativeURIs(tagName, propName) {
      var elems = articleContent.getElementsByTagName(tagName);
      for (var i = elems.length; --i >= 0;) {
        var elem = elems[i];
        var relativeURI = elem.getAttribute(propName);
        if (relativeURI != null)
          elems[i].setAttribute(propName, toAbsoluteURI(relativeURI));
      }
    }

     
    convertRelativeURIs("a", "href");

     
    convertRelativeURIs("img", "src");
  },

  




  _getArticleTitle: function() {
    var doc = this._doc;
    var curTitle = "";
    var origTitle = "";

    try {
      curTitle = origTitle = doc.title;

      
      if (typeof curTitle !== "string")
        curTitle = origTitle = this._getInnerText(doc.getElementsByTagName('title')[0]);
    } catch(e) {}

    if (curTitle.match(/ [\|\-] /)) {
      curTitle = origTitle.replace(/(.*)[\|\-] .*/gi,'$1');

      if (curTitle.split(' ').length < 3)
        curTitle = origTitle.replace(/[^\|\-]*[\|\-](.*)/gi,'$1');
    } else if (curTitle.indexOf(': ') !== -1) {
      curTitle = origTitle.replace(/.*:(.*)/gi, '$1');

      if (curTitle.split(' ').length < 3)
        curTitle = origTitle.replace(/[^:]*[:](.*)/gi,'$1');
    } else if (curTitle.length > 150 || curTitle.length < 15) {
      var hOnes = doc.getElementsByTagName('h1');

      if (hOnes.length === 1)
        curTitle = this._getInnerText(hOnes[0]);
    }

    curTitle = curTitle.replace(this.REGEXPS.trim, "");

    if (curTitle.split(' ').length <= 4)
      curTitle = origTitle;

    return curTitle;
  },

  





  _prepDocument: function() {
    var doc = this._doc;

    
    
    
    if (!doc.body) {
      var body = doc.createElement("body");

      try {
        doc.body = body;
      } catch(e) {
        doc.documentElement.appendChild(body);
        this.log(e);
      }
    }

    
    var styleTags = doc.getElementsByTagName("style");
    for (var st = 0; st < styleTags.length; st += 1) {
      styleTags[st].textContent = "";
    }

    this._replaceBrs(doc.body);

    var fonts = doc.getElementsByTagName("FONT");
    for (var i = fonts.length; --i >=0;) {
      this._setNodeTag(fonts[i], "SPAN");
    }
  },

  




  _nextElement: function (node) {
    var next = node;
    while (next
        && (next.nodeType != Node.ELEMENT_NODE)
        && this.REGEXPS.whitespace.test(next.textContent)) {
      next = next.nextSibling;
    }
    return next;
  },

  






  _replaceBrs: function (elem) {
    var brs = elem.getElementsByTagName("br");
    for (var i = 0; i < brs.length; i++) {
      var br = brs[i];
      var next = br.nextSibling;

      
      
      var replaced = false;

      
      
      
      while ((next = this._nextElement(next)) && (next.tagName == "BR")) {
        replaced = true;
        var sibling = next.nextSibling;
        next.parentNode.removeChild(next);
        next = sibling;
      }

      
      
      
      if (replaced) {
        var p = this._doc.createElement("p");
        br.parentNode.replaceChild(p, br);

        next = p.nextSibling;
        while (next) {
          
          if (next.tagName == "BR") {
            var nextElem = this._nextElement(next);
            if (nextElem && nextElem.tagName == "BR")
              break;
          }

          
          var sibling = next.nextSibling;
          p.appendChild(next);
          next = sibling;
        }
      }
    }
  },

  _setNodeTag: function (node, tag) {
    node.localName = tag.toLowerCase();
    node.tagName = tag.toUpperCase();
  },

  






  _prepArticle: function(articleContent) {
    this._cleanStyles(articleContent);

    
    this._cleanConditionally(articleContent, "form");
    this._clean(articleContent, "object");
    this._clean(articleContent, "h1");

    
    
    if (articleContent.getElementsByTagName('h2').length === 1)
      this._clean(articleContent, "h2");

    this._clean(articleContent, "iframe");
    this._cleanHeaders(articleContent);

    
    
    this._cleanConditionally(articleContent, "table");
    this._cleanConditionally(articleContent, "ul");
    this._cleanConditionally(articleContent, "div");

    
    var articleParagraphs = articleContent.getElementsByTagName('p');
    for (var i = articleParagraphs.length - 1; i >= 0; i -= 1) {
      var imgCount = articleParagraphs[i].getElementsByTagName('img').length;
      var embedCount = articleParagraphs[i].getElementsByTagName('embed').length;
      var objectCount = articleParagraphs[i].getElementsByTagName('object').length;

      if (imgCount === 0 &&
        embedCount === 0 &&
        objectCount === 0 &&
        this._getInnerText(articleParagraphs[i], false) === '')
        articleParagraphs[i].parentNode.removeChild(articleParagraphs[i]);
    }

    var brs = articleContent.getElementsByTagName("BR");
    for (var i = brs.length; --i >= 0;) {
      var br = brs[i];
      var next = this._nextElement(br.nextSibling);
      if (next && next.tagName == "P")
        br.parentNode.removeChild(br);
    }
  },

  






  _initializeNode: function(node) {
    node.readability = {"contentScore": 0};

    switch(node.tagName) {
      case 'DIV':
        node.readability.contentScore += 5;
        break;

      case 'PRE':
      case 'TD':
      case 'BLOCKQUOTE':
        node.readability.contentScore += 3;
        break;

      case 'ADDRESS':
      case 'OL':
      case 'UL':
      case 'DL':
      case 'DD':
      case 'DT':
      case 'LI':
      case 'FORM':
        node.readability.contentScore -= 3;
        break;

      case 'H1':
      case 'H2':
      case 'H3':
      case 'H4':
      case 'H5':
      case 'H6':
      case 'TH':
        node.readability.contentScore -= 5;
        break;
    }

    node.readability.contentScore += this._getClassWeight(node);
  },

  






  _grabArticle: function (page) {
    var doc = this._doc;
    var isPaging = (page !== null ? true: false);
    page = page ? page : this._doc.body;
    var pageCacheHtml = page.innerHTML;

    
    this._articleDir = doc.documentElement.getAttribute("dir");

    
    function purgeNode(node, allElements) {
      for (var i = node.childNodes.length; --i >= 0;) {
        purgeNode(node.childNodes[i], allElements);
      }
      if (node._index !== undefined && allElements[node._index] == node)
        delete allElements[node._index];
    }
    while (true) {
      var stripUnlikelyCandidates = this._flagIsActive(this.FLAG_STRIP_UNLIKELYS);
      var allElements = page.getElementsByTagName('*');

      
      
      
      
      
      
      var node = null;
      var nodesToScore = [];

      
      for (var i = allElements.length; --i >= 0;) {
        allElements[i]._index = i;
      }

      




      for (var nodeIndex = 0; nodeIndex < allElements.length; nodeIndex++) {
        if (!(node = allElements[nodeIndex]))
          continue;

        var matchString = node.className + node.id;
        if (matchString.search(this.REGEXPS.byline) !== -1 && !this._articleByline) {
          if (this._isValidByline(node.textContent)) {
            this._articleByline = node.textContent.trim();
            node.parentNode.removeChild(node);
            purgeNode(node, allElements);
            continue;
          }
        }

        
        if (stripUnlikelyCandidates) {
          if (matchString.search(this.REGEXPS.unlikelyCandidates) !== -1 &&
            matchString.search(this.REGEXPS.okMaybeItsACandidate) === -1 &&
            node.tagName !== "BODY") {
            this.log("Removing unlikely candidate - " + matchString);
            node.parentNode.removeChild(node);
            purgeNode(node, allElements);
            continue;
          }
        }

        if (node.tagName === "P" || node.tagName === "TD" || node.tagName === "PRE")
          nodesToScore[nodesToScore.length] = node;

        
        if (node.tagName === "DIV") {
          
          
          
          
          var pIndex = this._getSinglePIndexInsideDiv(node);

          if (pIndex >= 0 || !this._hasChildBlockElement(node)) {
            if (pIndex >= 0) {
              var newNode = node.childNodes[pIndex];
              node.parentNode.replaceChild(newNode, node);
              purgeNode(node, allElements);
            } else {
              this._setNodeTag(node, "P");
              nodesToScore[nodesToScore.length] = node;
            }
          } else {
            
            for (var i = 0, il = node.childNodes.length; i < il; i += 1) {
              var childNode = node.childNodes[i];
              if (!childNode)
                continue;

              if (childNode.nodeType === 3) { 
                var p = doc.createElement('p');
                p.textContent = childNode.textContent;
                p.style.display = 'inline';
                p.className = 'readability-styled';
                childNode.parentNode.replaceChild(p, childNode);
              }
            }
          }
        }
      }

      





      var candidates = [];
      for (var pt = 0; pt < nodesToScore.length; pt += 1) {
        var parentNode = nodesToScore[pt].parentNode;
        var grandParentNode = parentNode ? parentNode.parentNode : null;
        var innerText = this._getInnerText(nodesToScore[pt]);

        if (!parentNode || typeof(parentNode.tagName) === 'undefined')
          continue;

        
        if (innerText.length < 25)
          continue;

        
        if (typeof parentNode.readability === 'undefined') {
          this._initializeNode(parentNode);
          candidates.push(parentNode);
        }

        
        if (grandParentNode &&
          typeof(grandParentNode.readability) === 'undefined' &&
          typeof(grandParentNode.tagName) !== 'undefined') {
          this._initializeNode(grandParentNode);
          candidates.push(grandParentNode);
        }

        var contentScore = 0;

        
        contentScore += 1;

        
        contentScore += innerText.split(',').length;

        
        contentScore += Math.min(Math.floor(innerText.length / 100), 3);

        
        parentNode.readability.contentScore += contentScore;

        if (grandParentNode)
          grandParentNode.readability.contentScore += contentScore / 2;
      }

      
      
      var topCandidates = [];
      for (var c = 0, cl = candidates.length; c < cl; c += 1) {
        var candidate = candidates[c];

        
        
        
        var candidateScore = candidate.readability.contentScore * (1 - this._getLinkDensity(candidate));
        candidate.readability.contentScore = candidateScore;

        this.log('Candidate: ' + candidate + " (" + candidate.className + ":" +
          candidate.id + ") with score " + candidateScore);

        for (var t = 0; t < this.N_TOP_CANDIDATES; t++) {
          var aTopCandidate = topCandidates[t];

          if (!aTopCandidate || candidateScore > aTopCandidate.readability.contentScore) {
            topCandidates.splice(t, 0, candidate);
            if (topCandidates.length > this.N_TOP_CANDIDATES)
              topCandidates.pop();
            break;
          }
        }
      }

      var topCandidate = topCandidates[0] || null;

      
      
      if (topCandidate === null || topCandidate.tagName === "BODY") {
        
        topCandidate = doc.createElement("DIV");
        var children = page.childNodes;
        for (var i = 0; i < children.length; ++i) {
          topCandidate.appendChild(children[i]);
        }

        page.appendChild(topCandidate);

        this._initializeNode(topCandidate);
      }

      
      
      
      var articleContent = doc.createElement("DIV");
      if (isPaging)
        articleContent.id = "readability-content";

      var siblingScoreThreshold = Math.max(10, topCandidate.readability.contentScore * 0.2);
      var siblingNodes = topCandidate.parentNode.childNodes;

      for (var s = 0, sl = siblingNodes.length; s < sl; s += 1) {
        var siblingNode = siblingNodes[s];
        var append = false;

        this.log("Looking at sibling node: " + siblingNode + " (" + siblingNode.className + ":" + siblingNode.id + ")" + ((typeof siblingNode.readability !== 'undefined') ? (" with score " + siblingNode.readability.contentScore) : ''));
        this.log("Sibling has score " + (siblingNode.readability ? siblingNode.readability.contentScore : 'Unknown'));

        if (siblingNode === topCandidate)
          append = true;

        var contentBonus = 0;

        
        if (siblingNode.className === topCandidate.className && topCandidate.className !== "")
          contentBonus += topCandidate.readability.contentScore * 0.2;

        if (typeof siblingNode.readability !== 'undefined' &&
          (siblingNode.readability.contentScore+contentBonus) >= siblingScoreThreshold)
          append = true;

        if (siblingNode.nodeName === "P") {
          var linkDensity = this._getLinkDensity(siblingNode);
          var nodeContent = this._getInnerText(siblingNode);
          var nodeLength = nodeContent.length;

          if (nodeLength > 80 && linkDensity < 0.25) {
            append = true;
          } else if (nodeLength < 80 && linkDensity === 0 && nodeContent.search(/\.( |$)/) !== -1) {
            append = true;
          }
        }

        if (append) {
          this.log("Appending node: " + siblingNode);

          
          
          
          
          s -= 1;
          sl -= 1;

          if (siblingNode.nodeName !== "DIV" && siblingNode.nodeName !== "P") {
            
            
            this.log("Altering siblingNode of " + siblingNode.nodeName + ' to div.');

            this._setNodeTag(siblingNode, "DIV");
          }

          
          
          siblingNode.className = "";

          
          
          articleContent.appendChild(siblingNode);
        }
      }

      
      this._prepArticle(articleContent);

      if (this._curPageNum === 1) {
        var div = doc.createElement("DIV");
        div.id = "readability-page-1";
        div.className = "page";
        var children = articleContent.childNodes;
        for (var i = 0; i < children.length; ++i) {
          div.appendChild(children[i]);
        }
        articleContent.appendChild(div);
      }

      
      
      
      
      
      if (this._getInnerText(articleContent, true).length < 500) {
        page.innerHTML = pageCacheHtml;

        if (this._flagIsActive(this.FLAG_STRIP_UNLIKELYS)) {
          this._removeFlag(this.FLAG_STRIP_UNLIKELYS);
        } else if (this._flagIsActive(this.FLAG_WEIGHT_CLASSES)) {
          this._removeFlag(this.FLAG_WEIGHT_CLASSES);
        } else if (this._flagIsActive(this.FLAG_CLEAN_CONDITIONALLY)) {
          this._removeFlag(this.FLAG_CLEAN_CONDITIONALLY);
        } else {
          return null;
        }
      } else {
        return articleContent;
      }
    }
  },

  







  _isValidByline: function(byline) {
    if (typeof byline == 'string' || byline instanceof String) {
      byline = byline.trim();
      return (byline.length > 0) && (byline.length < 100);
    }
    return false;
  },

  












  _getExcerpt: function(articleContent) {
    var values = {};
    var metaElements = this._doc.getElementsByTagName("meta");

    
    
    var namePattern = /^\s*((twitter)\s*:\s*)?description\s*$/gi;

    
    var propertyPattern = /^\s*og\s*:\s*description\s*$/gi;

    
    for (var i = 0; i < metaElements.length; i++) {
      var element = metaElements[i];
      var elementName = element.getAttribute("name");
      var elementProperty = element.getAttribute("property");

      var name;
      if (namePattern.test(elementName)) {
        name = elementName;
      } else if (propertyPattern.test(elementProperty)) {
        name = elementProperty;
      }

      if (name) {
        var content = element.getAttribute("content");
        if (content) {
          
          
          name = name.toLowerCase().replace(/\s/g, '');
          values[name] = content.trim();
        }
      }
    }

    if ("description" in values) {
      return values["description"];
    }

    if ("og:description" in values) {
      
      return values["og:description"];
    }

    if ("twitter:description" in values) {
      
      return values["twitter:description"];
    }

    
    var paragraphs = articleContent.getElementsByTagName("p");
    if (paragraphs.length > 0) {
      return paragraphs[0].textContent;
    }

    return "";
  },

  




  _removeScripts: function(doc) {
    var scripts = doc.getElementsByTagName('script');
    for (var i = scripts.length - 1; i >= 0; i -= 1) {
      scripts[i].nodeValue="";
      scripts[i].removeAttribute('src');

      if (scripts[i].parentNode)
          scripts[i].parentNode.removeChild(scripts[i]);
    }
  },

  






  _getSinglePIndexInsideDiv: function(e) {
    var childNodes = e.childNodes;
    var pIndex = -1;

    for (var i = childNodes.length; --i >= 0;) {
      var node = childNodes[i];

      if (node.nodeType === Node.ELEMENT_NODE) {
        if (node.tagName !== "P")
          return -1;

        if (pIndex >= 0)
          return -1;

        pIndex = i;
      } else if (node.nodeType == Node.TEXT_NODE && this._getInnerText(node, false)) {
        return -1;
      }
    }

    return pIndex;
  },

  




  _hasChildBlockElement: function (e) {
    var length = e.childNodes.length;
    for (var i = 0; i < length; i++) {
      var child = e.childNodes[i];
      if (child.nodeType != 1)
        continue;

      if (this.DIV_TO_P_ELEMS.indexOf(child.tagName) !== -1 || this._hasChildBlockElement(child))
        return true;
    }
    return false;
  },

  






  _getInnerText: function(e, normalizeSpaces) {
    var textContent = e.textContent.replace(this.REGEXPS.trim, "");
    normalizeSpaces = (typeof normalizeSpaces === 'undefined') ? true : normalizeSpaces;

    if (normalizeSpaces) {
      return textContent.replace(this.REGEXPS.normalize, " ");
    } else {
      return textContent;
    }
  },

  






  _getCharCount: function(e,s) {
    s = s || ",";
    return this._getInnerText(e).split(s).length - 1;
  },

  






  _cleanStyles: function(e) {
    e = e || this._doc;
    var cur = e.firstChild;

    if (!e)
      return;

    
    if (typeof e.removeAttribute === 'function' && e.className !== 'readability-styled')
      e.removeAttribute('style');

    
    while (cur !== null) {
      if (cur.nodeType === 1) {
        
        if (cur.className !== "readability-styled")
          cur.removeAttribute("style");

        this._cleanStyles(cur);
      }

      cur = cur.nextSibling;
    }
  },

  






  _getLinkDensity: function(e) {
    var links = e.getElementsByTagName("a");
    var textLength = this._getInnerText(e).length;
    var linkLength = 0;

    for (var i = 0, il = links.length; i < il; i += 1) {
      linkLength += this._getInnerText(links[i]).length;
    }

    return linkLength / textLength;
  },

  





  _findBaseUrl: function() {
    var uri = this._uri;
    var noUrlParams = uri.path.split("?")[0];
    var urlSlashes = noUrlParams.split("/").reverse();
    var cleanedSegments = [];
    var possibleType = "";

    for (var i = 0, slashLen = urlSlashes.length; i < slashLen; i += 1) {
      var segment = urlSlashes[i];

      
      if (segment.indexOf(".") !== -1) {
        possibleType = segment.split(".")[1];

        
        if (!possibleType.match(/[^a-zA-Z]/))
          segment = segment.split(".")[0];
      }

      
      
      if (segment.indexOf(',00') !== -1)
        segment = segment.replace(',00', '');

      
      if (segment.match(/((_|-)?p[a-z]*|(_|-))[0-9]{1,2}$/i) && ((i === 1) || (i === 0)))
        segment = segment.replace(/((_|-)?p[a-z]*|(_|-))[0-9]{1,2}$/i, "");

      var del = false;

      
      
      if (i < 2 && segment.match(/^\d{1,2}$/))
        del = true;

      
      if (i === 0 && segment.toLowerCase() === "index")
        del = true;

      
      
      if (i < 2 && segment.length < 3 && !urlSlashes[0].match(/[a-z]/i))
        del = true;

      
      if (!del)
        cleanedSegments.push(segment);
    }

    
    return uri.scheme + "://" + uri.host + cleanedSegments.reverse().join("/");
  },

  





  _findNextPageLink: function(elem) {
    var uri = this._uri;
    var possiblePages = {};
    var allLinks = elem.getElementsByTagName('a');
    var articleBaseUrl = this._findBaseUrl();

    
    
    
    
    
    
    
    for (var i = 0, il = allLinks.length; i < il; i += 1) {
      var link = allLinks[i];
      var linkHref = allLinks[i].href.replace(/#.*$/, '').replace(/\/$/, '');

      
      if (linkHref === "" ||
        linkHref === articleBaseUrl ||
        linkHref === uri.spec ||
        linkHref in this._parsedPages) {
        continue;
      }

      
      if (uri.host !== linkHref.split(/\/+/g)[1])
        continue;

      var linkText = this._getInnerText(link);

      
      if (linkText.match(this.REGEXPS.extraneous) || linkText.length > 25)
        continue;

      
      
      var linkHrefLeftover = linkHref.replace(articleBaseUrl, '');
      if (!linkHrefLeftover.match(/\d/))
        continue;

      if (!(linkHref in possiblePages)) {
        possiblePages[linkHref] = {"score": 0, "linkText": linkText, "href": linkHref};
      } else {
        possiblePages[linkHref].linkText += ' | ' + linkText;
      }

      var linkObj = possiblePages[linkHref];

      
      
      
      if (linkHref.indexOf(articleBaseUrl) !== 0)
        linkObj.score -= 25;

      var linkData = linkText + ' ' + link.className + ' ' + link.id;
      if (linkData.match(this.REGEXPS.nextLink))
        linkObj.score += 50;

      if (linkData.match(/pag(e|ing|inat)/i))
        linkObj.score += 25;

      if (linkData.match(/(first|last)/i)) {
        
        
        
        if (!linkObj.linkText.match(this.REGEXPS.nextLink))
          linkObj.score -= 65;
      }

      if (linkData.match(this.REGEXPS.negative) || linkData.match(this.REGEXPS.extraneous))
        linkObj.score -= 50;

      if (linkData.match(this.REGEXPS.prevLink))
        linkObj.score -= 200;

      
      var parentNode = link.parentNode;
      var positiveNodeMatch = false;
      var negativeNodeMatch = false;

      while (parentNode) {
        var parentNodeClassAndId = parentNode.className + ' ' + parentNode.id;

        if (!positiveNodeMatch && parentNodeClassAndId && parentNodeClassAndId.match(/pag(e|ing|inat)/i)) {
          positiveNodeMatch = true;
          linkObj.score += 25;
        }

        if (!negativeNodeMatch && parentNodeClassAndId && parentNodeClassAndId.match(this.REGEXPS.negative)) {
          
          
          if (!parentNodeClassAndId.match(this.REGEXPS.positive)) {
            linkObj.score -= 25;
            negativeNodeMatch = true;
          }
        }

        parentNode = parentNode.parentNode;
      }

      
      
      if (linkHref.match(/p(a|g|ag)?(e|ing|ination)?(=|\/)[0-9]{1,2}/i) || linkHref.match(/(page|paging)/i))
        linkObj.score += 25;

      
      if (linkHref.match(this.REGEXPS.extraneous))
        linkObj.score -= 15;

      








      
      
      
      var linkTextAsNumber = parseInt(linkText, 10);
      if (linkTextAsNumber) {
        
        
        if (linkTextAsNumber === 1) {
          linkObj.score -= 10;
        } else {
          linkObj.score += Math.max(0, 10 - linkTextAsNumber);
        }
      }
    }

    
    
    
    var topPage = null;
    for (var page in possiblePages) {
      if (possiblePages.hasOwnProperty(page)) {
        if (possiblePages[page].score >= 50 &&
          (!topPage || topPage.score < possiblePages[page].score))
          topPage = possiblePages[page];
      }
    }

    if (topPage) {
      var nextHref = topPage.href.replace(/\/$/,'');

      this.log('NEXT PAGE IS ' + nextHref);
      this._parsedPages[nextHref] = true;
      return nextHref;
    } else {
      return null;
    }
  },

  _successfulRequest: function(request) {
    return (request.status >= 200 && request.status < 300) ||
        request.status === 304 ||
         (request.status === 0 && request.responseText);
  },

  _ajax: function(url, options) {
    var request = new XMLHttpRequest();

    function respondToReadyState(readyState) {
      if (request.readyState === 4) {
        if (this._successfulRequest(request)) {
          if (options.success)
            options.success(request);
        } else {
          if (options.error)
            options.error(request);
        }
      }
    }

    if (typeof options === 'undefined')
      options = {};

    request.onreadystatechange = respondToReadyState;

    request.open('get', url, true);
    request.setRequestHeader('Accept', 'text/html');

    try {
      request.send(options.postBody);
    } catch (e) {
      if (options.error)
        options.error();
    }

    return request;
  },

  _appendNextPage: function(nextPageLink) {
    var doc = this._doc;
    this._curPageNum += 1;

    var articlePage = doc.createElement("DIV");
    articlePage.id = 'readability-page-' + this._curPageNum;
    articlePage.className = 'page';
    articlePage.innerHTML = '<p class="page-separator" title="Page ' + this._curPageNum + '">&sect;</p>';

    doc.getElementById("readability-content").appendChild(articlePage);

    if (this._curPageNum > this.MAX_PAGES) {
      var nextPageMarkup = "<div style='text-align: center'><a href='" + nextPageLink + "'>View Next Page</a></div>";
      articlePage.innerHTML = articlePage.innerHTML + nextPageMarkup;
      return;
    }

    
    
    (function(pageUrl, thisPage) {
      this._ajax(pageUrl, {
        success: function(r) {

          
          var eTag = r.getResponseHeader('ETag');
          if (eTag) {
            if (eTag in this._pageETags) {
              this.log("Exact duplicate page found via ETag. Aborting.");
              articlePage.style.display = 'none';
              return;
            } else {
              this._pageETags[eTag] = 1;
            }
          }

          
          var page = doc.createElement("DIV");

          
          
          
          
          
          
          
          
          var responseHtml = r.responseText.replace(/\n/g,'\uffff').replace(/<script.*?>.*?<\/script>/gi, '');
          responseHtml = responseHtml.replace(/\n/g,'\uffff').replace(/<script.*?>.*?<\/script>/gi, '');
          responseHtml = responseHtml.replace(/\uffff/g,'\n').replace(/<(\/?)noscript/gi, '<$1div');
          responseHtml = responseHtml.replace(this.REGEXPS.replaceFonts, '<$1span>');

          page.innerHTML = responseHtml;
          this._replaceBrs(page);

          
          
          this._flags = 0x1 | 0x2 | 0x4;

          var nextPageLink = this._findNextPageLink(page);

          
          
          var content = this._grabArticle(page);

          if (!content) {
            this.log("No content found in page to append. Aborting.");
            return;
          }

          
          
          
          var firstP = content.getElementsByTagName("P").length ? content.getElementsByTagName("P")[0] : null;
          if (firstP && firstP.innerHTML.length > 100) {
            for (var i = 1; i <= this._curPageNum; i += 1) {
              var rPage = doc.getElementById('readability-page-' + i);
              if (rPage && rPage.innerHTML.indexOf(firstP.innerHTML) !== -1) {
                this.log('Duplicate of page ' + i + ' - skipping.');
                articlePage.style.display = 'none';
                this._parsedPages[pageUrl] = true;
                return;
              }
            }
          }

          this._removeScripts(content);

          thisPage.innerHTML = thisPage.innerHTML + content.innerHTML;

          
          
          
          setTimeout((function() {
            this._postProcessContent(thisPage);
          }).bind(this), 500);


          if (nextPageLink)
            this._appendNextPage(nextPageLink);
        }
      });
    }).bind(this)(nextPageLink, articlePage);
  },

  






  _getClassWeight: function(e) {
    if (!this._flagIsActive(this.FLAG_WEIGHT_CLASSES))
      return 0;

    var weight = 0;

    
    if (typeof(e.className) === 'string' && e.className !== '') {
      if (e.className.search(this.REGEXPS.negative) !== -1)
        weight -= 25;

      if (e.className.search(this.REGEXPS.positive) !== -1)
        weight += 25;
    }

    
    if (typeof(e.id) === 'string' && e.id !== '') {
      if (e.id.search(this.REGEXPS.negative) !== -1)
        weight -= 25;

      if (e.id.search(this.REGEXPS.positive) !== -1)
        weight += 25;
    }

    return weight;
  },

  







  _clean: function(e, tag) {
    var targetList = e.getElementsByTagName(tag);
    var isEmbed = (tag === 'object' || tag === 'embed');

    for (var y = targetList.length - 1; y >= 0; y -= 1) {
      
      if (isEmbed) {
        var attributeValues = "";
        for (var i = 0, il = targetList[y].attributes.length; i < il; i += 1) {
          attributeValues += targetList[y].attributes[i].value + '|';
        }

        
        if (attributeValues.search(this.REGEXPS.videos) !== -1)
          continue;

        
        if (targetList[y].innerHTML.search(this.REGEXPS.videos) !== -1)
          continue;
      }

      targetList[y].parentNode.removeChild(targetList[y]);
    }
  },

  





  _cleanConditionally: function(e, tag) {
    if (!this._flagIsActive(this.FLAG_CLEAN_CONDITIONALLY))
      return;

    var tagsList = e.getElementsByTagName(tag);
    var curTagsLength = tagsList.length;

    
    
    
    
    
    for (var i = curTagsLength-1; i >= 0; i -= 1) {
      var weight = this._getClassWeight(tagsList[i]);
      var contentScore = 0;

      this.log("Cleaning Conditionally " + tagsList[i] + " (" + tagsList[i].className + ":" + tagsList[i].id + ")");

      if (weight + contentScore < 0) {
        tagsList[i].parentNode.removeChild(tagsList[i]);
      } else if (this._getCharCount(tagsList[i],',') < 10) {
        
        
        
        var p = tagsList[i].getElementsByTagName("p").length;
        var img = tagsList[i].getElementsByTagName("img").length;
        var li = tagsList[i].getElementsByTagName("li").length-100;
        var input = tagsList[i].getElementsByTagName("input").length;

        var embedCount = 0;
        var embeds = tagsList[i].getElementsByTagName("embed");
        for (var ei = 0, il = embeds.length; ei < il; ei += 1) {
          if (embeds[ei].src.search(this.REGEXPS.videos) === -1)
            embedCount += 1;
        }

        var linkDensity = this._getLinkDensity(tagsList[i]);
        var contentLength = this._getInnerText(tagsList[i]).length;
        var toRemove = false;

        if (img > p) {
          toRemove = true;
        } else if (li > p && tag !== "ul" && tag !== "ol") {
          toRemove = true;
        } else if ( input > Math.floor(p/3) ) {
          toRemove = true;
        } else if (contentLength < 25 && (img === 0 || img > 2) ) {
          toRemove = true;
        } else if (weight < 25 && linkDensity > 0.2) {
          toRemove = true;
        } else if (weight >= 25 && linkDensity > 0.5) {
          toRemove = true;
        } else if ((embedCount === 1 && contentLength < 75) || embedCount > 1) {
          toRemove = true;
        }

        if (toRemove)
          tagsList[i].parentNode.removeChild(tagsList[i]);
      }
    }
  },

  





  _cleanHeaders: function(e) {
    for (var headerIndex = 1; headerIndex < 3; headerIndex += 1) {
      var headers = e.getElementsByTagName('h' + headerIndex);
      for (var i = headers.length - 1; i >= 0; i -= 1) {
        if (this._getClassWeight(headers[i]) < 0 || this._getLinkDensity(headers[i]) > 0.33)
          headers[i].parentNode.removeChild(headers[i]);
      }
    }
  },

  _flagIsActive: function(flag) {
    return (this._flags & flag) > 0;
  },

  _addFlag: function(flag) {
    this._flags = this._flags | flag;
  },

  _removeFlag: function(flag) {
    this._flags = this._flags & ~flag;
  },

  











  parse: function () {
    
    this._removeScripts(this._doc);

    
    

    
    
    

    
    

    this._prepDocument();

    var articleTitle = this._getArticleTitle();
    var articleContent = this._grabArticle();
    if (!articleContent)
      return null;

    this._postProcessContent(articleContent);

    
    
    
    
    
    
    

    var excerpt = this._getExcerpt(articleContent);

    return { uri: this._uri,
             title: articleTitle,
             byline: this._articleByline,
             dir: this._articleDir,
             content: articleContent.innerHTML,
             length: articleContent.textContent.length,
             excerpt: excerpt };
  }
};
