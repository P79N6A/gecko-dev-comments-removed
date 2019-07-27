


























var root = this;







var Readability = function(uri, doc, options) {
  options = options || {};

  this._uri = uri;
  this._doc = doc;
  this._biggestFrame = false;
  this._articleByline = null;
  this._articleDir = null;

  
  this._debug = !!options.debug;
  this._maxElemsToParse = options.maxElemsToParse || this.DEFAULT_MAX_ELEMS_TO_PARSE;
  this._nbTopCandidates = options.nbTopCandidates || this.DEFAULT_N_TOP_CANDIDATES;
  this._maxPages = options.maxPages || this.DEFAULT_MAX_PAGES;

  
  this._flags = this.FLAG_STRIP_UNLIKELYS |
                this.FLAG_WEIGHT_CLASSES |
                this.FLAG_CLEAN_CONDITIONALLY;

  
  
  this._parsedPages = {};

  
  
  this._pageETags = {};

  
  this._curPageNum = 1;

  
  if (this._debug) {
    function logEl(e) {
      var rv = e.nodeName + " ";
      if (e.nodeType == e.TEXT_NODE) {
        return rv + '("' + e.textContent + '")';
      }
      var classDesc = e.className && ("." + e.className.replace(/ /g, "."));
      var elDesc = e.id ? "(#" + e.id + classDesc + ")" :
                          (classDesc ? "(" + classDesc + ")" : "");
      return rv + elDesc;
    }
    this.log = function () {
      if ("dump" in root) {
        var msg = Array.prototype.map.call(arguments, function(x) {
          return (x && x.nodeName) ? logEl(x) : x;
        }).join(" ");
        dump("Reader: (Readability) " + msg + "\n");
      } else if ("console" in root) {
        var args = ["Reader: (Readability) "].concat(arguments);
        console.log.apply(console, args);
      }
    };
  } else {
    this.log = function () {};
  }
}

Readability.prototype = {
  FLAG_STRIP_UNLIKELYS: 0x1,
  FLAG_WEIGHT_CLASSES: 0x2,
  FLAG_CLEAN_CONDITIONALLY: 0x4,

  
  DEFAULT_MAX_ELEMS_TO_PARSE: 0,

  
  
  DEFAULT_N_TOP_CANDIDATES: 5,

  
  
  DEFAULT_MAX_PAGES: 5,

  
  
  REGEXPS: {
    unlikelyCandidates: /banner|combx|comment|community|disqus|extra|foot|header|menu|remark|rss|share|shoutbox|sidebar|skyscraper|sponsor|ad-break|agegate|pagination|pager|popup/i,
    okMaybeItsACandidate: /and|article|body|column|main|shadow/i,
    positive: /article|body|content|entry|hentry|main|page|pagination|post|text|blog|story/i,
    negative: /hidden|banner|combx|comment|com-|contact|foot|footer|footnote|masthead|media|meta|outbrain|promo|related|scroll|share|shoutbox|sidebar|skyscraper|sponsor|shopping|tags|tool|widget/i,
    extraneous: /print|archive|comment|discuss|e[\-]?mail|share|reply|all|login|sign|single|utility/i,
    byline: /byline|author|dateline|writtenby/i,
    replaceFonts: /<(\/?)font[^>]*>/gi,
    normalize: /\s{2,}/g,
    videos: /https?:\/\/(www\.)?(youtube|youtube-nocookie|player\.vimeo)\.com/i,
    nextLink: /(next|weiter|continue|>([^\|]|$)|»([^\|]|$))/i,
    prevLink: /(prev|earl|old|new|<|«)/i,
    whitespace: /^\s*$/,
    hasContent: /\S$/,
  },

  DIV_TO_P_ELEMS: [ "A", "BLOCKQUOTE", "DL", "DIV", "IMG", "OL", "P", "PRE", "TABLE", "UL", "SELECT" ],

  ALTER_TO_DIV_EXCEPTIONS: ["DIV", "ARTICLE", "SECTION", "P"],

  





  _postProcessContent: function(articleContent) {
    
    this._fixRelativeUris(articleContent);
  },

  










  _forEachNode: function(nodeList, fn) {
    return Array.prototype.forEach.call(nodeList, fn, this);
  },

  










  _someNode: function(nodeList, fn) {
    return Array.prototype.some.call(nodeList, fn, this);
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

      
      if (uri.indexOf("./") === 0)
        return pathBase + uri.slice(2);

      
      
      return pathBase + uri;
    }

    var links = articleContent.getElementsByTagName("a");
    this._forEachNode(links, function(link) {
      var href = link.getAttribute("href");
      if (href) {
        
        
        if (href.indexOf("javascript:") === 0) {
          var text = this._doc.createTextNode(link.textContent);
          link.parentNode.replaceChild(text, link);
        } else {
          link.setAttribute("href", toAbsoluteURI(href));
        }
      }
    });

    var imgs = articleContent.getElementsByTagName("img");
    this._forEachNode(imgs, function(img) {
      var src = img.getAttribute("src");
      if (src) {
        img.setAttribute("src", toAbsoluteURI(src));
      }
    });
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

    curTitle = curTitle.trim();

    if (curTitle.split(' ').length <= 4)
      curTitle = origTitle;

    return curTitle;
  },

  





  _prepDocument: function() {
    var doc = this._doc;

    
    this._forEachNode(doc.getElementsByTagName("style"), function(styleNode) {
      styleNode.parentNode.removeChild(styleNode);
    });

    if (doc.body) {
      this._replaceBrs(doc.body);
    }

    this._forEachNode(doc.getElementsByTagName("font"), function(fontNode) {
      this._setNodeTag(fontNode, "SPAN");
    });
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
    this._forEachNode(elem.getElementsByTagName("br"), function(br) {
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
    });
  },

  _setNodeTag: function (node, tag) {
    this.log("_setNodeTag", node, tag);
    if (node.__JSDOMParser__) {
      node.localName = tag.toLowerCase();
      node.tagName = tag.toUpperCase();
      return node;
    }

    var replacement = node.ownerDocument.createElement(tag);
    while (node.firstChild) {
      replacement.appendChild(node.firstChild);
    }
    node.parentNode.replaceChild(replacement, node);
    if (node.readability)
      replacement.readability = node.readability;

    for (var i = 0; i < node.attributes.length; i++) {
      replacement.setAttribute(node.attributes[i].name, node.attributes[i].value);
    }
    return replacement;
  },

  






  _prepArticle: function(articleContent) {
    this._cleanStyles(articleContent);

    
    this._cleanConditionally(articleContent, "form");
    this._clean(articleContent, "object");
    this._clean(articleContent, "embed");
    this._clean(articleContent, "h1");

    
    
    if (articleContent.getElementsByTagName('h2').length === 1)
      this._clean(articleContent, "h2");

    this._clean(articleContent, "iframe");
    this._cleanHeaders(articleContent);

    
    
    this._cleanConditionally(articleContent, "table");
    this._cleanConditionally(articleContent, "ul");
    this._cleanConditionally(articleContent, "div");

    
    this._forEachNode(articleContent.getElementsByTagName('p'), function(paragraph) {
      var imgCount = paragraph.getElementsByTagName('img').length;
      var embedCount = paragraph.getElementsByTagName('embed').length;
      var objectCount = paragraph.getElementsByTagName('object').length;
      
      var iframeCount = paragraph.getElementsByTagName('iframe').length;
      var totalCount = imgCount + embedCount + objectCount + iframeCount;

      if (totalCount === 0 && !this._getInnerText(paragraph, false))
        paragraph.parentNode.removeChild(paragraph);
    });

    this._forEachNode(articleContent.getElementsByTagName("br"), function(br) {
      var next = this._nextElement(br.nextSibling);
      if (next && next.tagName == "P")
        br.parentNode.removeChild(br);
    });
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

  _removeAndGetNext: function(node) {
    var nextNode = this._getNextNode(node, true);
    node.parentNode.removeChild(node);
    return nextNode;
  },

  






  _getNextNode: function(node, ignoreSelfAndKids) {
    
    if (!ignoreSelfAndKids && node.firstElementChild) {
      return node.firstElementChild;
    }
    
    if (node.nextElementSibling) {
      return node.nextElementSibling;
    }
    
    
    
    do {
      node = node.parentNode;
    } while (node && !node.nextElementSibling);
    return node && node.nextElementSibling;
  },

  



  _getNextNodeNoElementProperties: function(node, ignoreSelfAndKids) {
    function nextSiblingEl(n) {
      do {
        n = n.nextSibling;
      } while (n && n.nodeType !== n.ELEMENT_NODE);
      return n;
    }
    
    if (!ignoreSelfAndKids && node.children[0]) {
      return node.children[0];
    }
    
    var next = nextSiblingEl(node);
    if (next) {
      return next;
    }
    
    
    
    do {
      node = node.parentNode;
      if (node)
        next = nextSiblingEl(node);
    } while (node && !next);
    return node && next;
  },

  _checkByline: function(node, matchString) {
    if (this._articleByline) {
      return false;
    }

    if (node.getAttribute !== undefined) {
      var rel = node.getAttribute("rel");
    }

    if ((rel === "author" || this.REGEXPS.byline.test(matchString)) && this._isValidByline(node.textContent)) {
      this._articleByline = node.textContent.trim();
      return true;
    }

    return false;
  },

  






  _grabArticle: function (page) {
    this.log("**** grabArticle ****");
    var doc = this._doc;
    var isPaging = (page !== null ? true: false);
    page = page ? page : this._doc.body;

    
    if (!page) {
      this.log("No body found in document. Abort.");
      return null;
    }

    var pageCacheHtml = page.innerHTML;

    
    this._articleDir = doc.documentElement.getAttribute("dir");

    while (true) {
      var stripUnlikelyCandidates = this._flagIsActive(this.FLAG_STRIP_UNLIKELYS);

      
      
      
      var elementsToScore = [];
      var node = this._doc.documentElement;

      while (node) {
        var matchString = node.className + " " + node.id;

        
        if (this._checkByline(node, matchString)) {
          node = this._removeAndGetNext(node);
          continue;
        }

        
        if (stripUnlikelyCandidates) {
          if (this.REGEXPS.unlikelyCandidates.test(matchString) &&
              !this.REGEXPS.okMaybeItsACandidate.test(matchString) &&
              node.tagName !== "BODY" &&
              node.tagName !== "A") {
            this.log("Removing unlikely candidate - " + matchString);
            node = this._removeAndGetNext(node);
            continue;
          }
        }

        if (node.tagName === "P" || node.tagName === "TD" || node.tagName === "PRE")
          elementsToScore.push(node);

        
        if (node.tagName === "DIV") {
          
          
          
          
          if (this._hasSinglePInsideElement(node)) {
            var newNode = node.children[0];
            node.parentNode.replaceChild(newNode, node);
            node = newNode;
          } else if (!this._hasChildBlockElement(node)) {
            node = this._setNodeTag(node, "P");
            elementsToScore.push(node);
          } else {
            
            this._forEachNode(node.childNodes, function(childNode) {
              if (childNode.nodeType === Node.TEXT_NODE) {
                var p = doc.createElement('p');
                p.textContent = childNode.textContent;
                p.style.display = 'inline';
                p.className = 'readability-styled';
                node.replaceChild(p, childNode);
              }
            });
          }
        }
        node = this._getNextNode(node);
      }

      





      var candidates = [];
      this._forEachNode(elementsToScore, function(elementToScore) {
        var parentNode = elementToScore.parentNode;
        var grandParentNode = parentNode ? parentNode.parentNode : null;
        var innerText = this._getInnerText(elementToScore);

        if (!parentNode || typeof(parentNode.tagName) === 'undefined')
          return;

        
        if (innerText.length < 25)
          return;

        
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
      });

      
      
      var topCandidates = [];
      for (var c = 0, cl = candidates.length; c < cl; c += 1) {
        var candidate = candidates[c];

        
        
        
        var candidateScore = candidate.readability.contentScore * (1 - this._getLinkDensity(candidate));
        candidate.readability.contentScore = candidateScore;

        this.log('Candidate:', candidate, "with score " + candidateScore);

        for (var t = 0; t < this._nbTopCandidates; t++) {
          var aTopCandidate = topCandidates[t];

          if (!aTopCandidate || candidateScore > aTopCandidate.readability.contentScore) {
            topCandidates.splice(t, 0, candidate);
            if (topCandidates.length > this._nbTopCandidates)
              topCandidates.pop();
            break;
          }
        }
      }

      var topCandidate = topCandidates[0] || null;
      var neededToCreateTopCandidate = false;

      
      
      if (topCandidate === null || topCandidate.tagName === "BODY") {
        
        topCandidate = doc.createElement("DIV");
        neededToCreateTopCandidate = true;
        
        
        var kids = page.childNodes;
        while (kids.length) {
          this.log("Moving child out:", kids[0]);
          topCandidate.appendChild(kids[0]);
        }

        page.appendChild(topCandidate);

        this._initializeNode(topCandidate);
      } else if (topCandidate) {
        
        
        
        
        
        
        
        var parentOfTopCandidate = topCandidate.parentNode;
        var lastScore = topCandidate.readability.contentScore;
        
        var scoreThreshold = lastScore / 3;
        while (parentOfTopCandidate && parentOfTopCandidate.readability) {
          var parentScore = parentOfTopCandidate.readability.contentScore;
          if (parentScore < scoreThreshold)
            break;
          if (parentScore > lastScore) {
            
            topCandidate = parentOfTopCandidate;
            break;
          }
          lastScore = parentOfTopCandidate.readability.contentScore;
          parentOfTopCandidate = parentOfTopCandidate.parentNode;
        }
      }

      
      
      
      var articleContent = doc.createElement("DIV");
      if (isPaging)
        articleContent.id = "readability-content";

      var siblingScoreThreshold = Math.max(10, topCandidate.readability.contentScore * 0.2);
      var siblings = topCandidate.parentNode.children;

      for (var s = 0, sl = siblings.length; s < sl; s++) {
        var sibling = siblings[s];
        var append = false;

        this.log("Looking at sibling node:", sibling, sibling.readability ? ("with score " + sibling.readability.contentScore) : '');
        this.log("Sibling has score", sibling.readability ? sibling.readability.contentScore : 'Unknown');

        if (sibling === topCandidate) {
          append = true;
        } else {
          var contentBonus = 0;

          
          if (sibling.className === topCandidate.className && topCandidate.className !== "")
            contentBonus += topCandidate.readability.contentScore * 0.2;

          if (sibling.readability &&
              ((sibling.readability.contentScore + contentBonus) >= siblingScoreThreshold)) {
            append = true;
          } else if (sibling.nodeName === "P") {
            var linkDensity = this._getLinkDensity(sibling);
            var nodeContent = this._getInnerText(sibling);
            var nodeLength = nodeContent.length;

            if (nodeLength > 80 && linkDensity < 0.25) {
              append = true;
            } else if (nodeLength < 80 && linkDensity === 0 && nodeContent.search(/\.( |$)/) !== -1) {
              append = true;
            }
          }
        }

        if (append) {
          this.log("Appending node:", sibling);

          if (this.ALTER_TO_DIV_EXCEPTIONS.indexOf(sibling.nodeName) === -1) {
            
            
            this.log("Altering sibling:", sibling, 'to div.');

            sibling = this._setNodeTag(sibling, "DIV");
          }

          
          
          sibling.removeAttribute("class");

          articleContent.appendChild(sibling);
          
          
          
          
          s -= 1;
          sl -= 1;
        }
      }

      if (this._debug)
        this.log("Article content pre-prep: " + articleContent.innerHTML);
      
      this._prepArticle(articleContent);
      if (this._debug)
        this.log("Article content post-prep: " + articleContent.innerHTML);

      if (this._curPageNum === 1) {
        if (neededToCreateTopCandidate) {
          
          
          
          
          topCandidate.id = "readability-page-1";
          topCandidate.className = "page";
        } else {
          var div = doc.createElement("DIV");
          div.id = "readability-page-1";
          div.className = "page";
          var children = articleContent.childNodes;
          while (children.length) {
            div.appendChild(children[0]);
          }
          articleContent.appendChild(div);
        }
      }

      if (this._debug)
        this.log("Article content after paging: " + articleContent.innerHTML);

      
      
      
      
      
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

  




  _getArticleMetadata: function() {
    var metadata = {};
    var values = {};
    var metaElements = this._doc.getElementsByTagName("meta");

    
    
    var namePattern = /^\s*((twitter)\s*:\s*)?description\s*$/gi;

    
    var propertyPattern = /^\s*og\s*:\s*description\s*$/gi;

    
    this._forEachNode(metaElements, function(element) {
      var elementName = element.getAttribute("name");
      var elementProperty = element.getAttribute("property");

      if (elementName === "author") {
        metadata.byline = element.getAttribute("content");
        return;
      }

      var name = null;
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
    });

    if ("description" in values) {
      metadata.excerpt = values["description"];
    } else if ("og:description" in values) {
      
      metadata.excerpt = values["og:description"];
    } else if ("twitter:description" in values) {
      
      metadata.excerpt = values["twitter:description"];
    }

    return metadata;
  },

  




  _removeScripts: function(doc) {
    this._forEachNode(doc.getElementsByTagName('script'), function(scriptNode) {
      scriptNode.nodeValue = "";
      scriptNode.removeAttribute('src');

      if (scriptNode.parentNode)
        scriptNode.parentNode.removeChild(scriptNode);
    });
    this._forEachNode(doc.getElementsByTagName('noscript'), function(noscriptNode) {
      if (noscriptNode.parentNode)
        noscriptNode.parentNode.removeChild(noscriptNode);
    });
  },

  






  _hasSinglePInsideElement: function(element) {
    
    if (element.children.length != 1 || element.children[0].tagName !== "P") {
      return false;
    }

    
    return !this._someNode(element.childNodes, function(node) {
      return node.nodeType === Node.TEXT_NODE &&
             this.REGEXPS.hasContent.test(node.textContent);
    });
  },

  




  _hasChildBlockElement: function (element) {
    return this._someNode(element.childNodes, function(node) {
      return this.DIV_TO_P_ELEMS.indexOf(node.tagName) !== -1 ||
             this._hasChildBlockElement(node);
    });
  },

  







  _getInnerText: function(e, normalizeSpaces) {
    normalizeSpaces = (typeof normalizeSpaces === 'undefined') ? true : normalizeSpaces;
    var textContent = e.textContent.trim();

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
    if (!e)
      return;
    var cur = e.firstChild;

    
    if (typeof e.removeAttribute === 'function' && e.className !== 'readability-styled')
      e.removeAttribute('style');

    
    while (cur !== null) {
      if (cur.nodeType === cur.ELEMENT_NODE) {
        
        if (cur.className !== "readability-styled")
          cur.removeAttribute("style");

        this._cleanStyles(cur);
      }

      cur = cur.nextSibling;
    }
  },

  






  _getLinkDensity: function(element) {
    var textLength = this._getInnerText(element).length;
    if (textLength === 0)
      return;

    var linkLength = 0;

    
    this._forEachNode(element.getElementsByTagName("a"), function(linkNode) {
      linkLength += this._getInnerText(linkNode).length;
    });

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

    if (this._curPageNum > this._maxPages) {
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
      if (this.REGEXPS.negative.test(e.className))
        weight -= 25;

      if (this.REGEXPS.positive.test(e.className))
        weight += 25;
    }

    
    if (typeof(e.id) === 'string' && e.id !== '') {
      if (this.REGEXPS.negative.test(e.id))
        weight -= 25;

      if (this.REGEXPS.positive.test(e.id))
        weight += 25;
    }

    return weight;
  },

  







  _clean: function(e, tag) {
    var isEmbed = ["object", "embed", "iframe"].indexOf(tag) !== -1;

    this._forEachNode(e.getElementsByTagName(tag), function(element) {
      
      if (isEmbed) {
        var attributeValues = [].map.call(element.attributes, function(attr) {
          return attr.value;
        }).join("|");

        
        if (this.REGEXPS.videos.test(attributeValues))
          return;

        
        if (this.REGEXPS.videos.test(element.innerHTML))
          return;
      }

      element.parentNode.removeChild(element);
    });
  },

  







  _hasAncestorTag: function(node, tagName, maxDepth) {
    maxDepth = maxDepth || 3;
    tagName = tagName.toUpperCase();
    var depth = 0;
    while (node.parentNode) {
      if (depth > maxDepth)
        return false;
      if (node.parentNode.tagName === tagName)
        return true;
      node = node.parentNode;
      depth++;
    }
    return false;
  },

  





  _cleanConditionally: function(e, tag) {
    if (!this._flagIsActive(this.FLAG_CLEAN_CONDITIONALLY))
      return;

    var tagsList = e.getElementsByTagName(tag);
    var curTagsLength = tagsList.length;

    
    
    
    
    
    for (var i = curTagsLength-1; i >= 0; i -= 1) {
      var weight = this._getClassWeight(tagsList[i]);
      var contentScore = 0;

      this.log("Cleaning Conditionally", tagsList[i]);

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
          if (!this.REGEXPS.videos.test(embeds[ei].src))
            embedCount += 1;
        }

        var linkDensity = this._getLinkDensity(tagsList[i]);
        var contentLength = this._getInnerText(tagsList[i]).length;
        var toRemove = false;
        if (img > p && !this._hasAncestorTag(tagsList[i], "figure")) {
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

        if (toRemove) {
          tagsList[i].parentNode.removeChild(tagsList[i]);
        }
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

  




  isProbablyReaderable: function() {
    var nodes = this._doc.getElementsByTagName("p");
    if (nodes.length < 5) {
      return false;
    }

    var possibleParagraphs = 0;
    for (var i = 0; i < nodes.length; i++) {
      var node = nodes[i];
      var matchString = node.className + " " + node.id;

      if (this.REGEXPS.unlikelyCandidates.test(matchString) &&
          !this.REGEXPS.okMaybeItsACandidate.test(matchString)) {
        continue;
      }

      if (node.textContent.trim().length < 100) {
        continue;
      }

      possibleParagraphs++;
      if (possibleParagraphs >= 5) {
        return true;
      }
    }
    return false;
  },

  











  parse: function () {
    
    if (this._maxElemsToParse > 0) {
      var numTags = this._doc.getElementsByTagName("*").length;
      if (numTags > this._maxElemsToParse) {
        throw new Error("Aborting parsing document; " + numTags + " elements found");
      }
    }

    if (typeof this._doc.documentElement.firstElementChild === "undefined") {
      this._getNextNode = this._getNextNodeNoElementProperties;
    }
    
    this._removeScripts(this._doc);

    
    

    
    
    

    
    

    this._prepDocument();

    var articleTitle = this._getArticleTitle();
    var metadata = this._getArticleMetadata();

    var articleContent = this._grabArticle();
    if (!articleContent)
      return null;

    this.log("Grabbed: " + articleContent.innerHTML);

    this._postProcessContent(articleContent);

    
    
    
    
    
    
    

    
    
    
    if (!metadata.excerpt) {
      var paragraphs = articleContent.getElementsByTagName("p");
      if (paragraphs.length > 0) {
        metadata.excerpt = paragraphs[0].textContent.trim();
      }
    }

    return { uri: this._uri,
             title: articleTitle,
             byline: metadata.byline || this._articleByline,
             dir: this._articleDir,
             content: articleContent.innerHTML,
             length: articleContent.textContent.length,
             excerpt: metadata.excerpt };
  }
};
