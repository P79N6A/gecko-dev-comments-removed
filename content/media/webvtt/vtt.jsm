



this.EXPORTED_SYMBOLS = ["WebVTTParser"];








(function(global) {

  
  function parseTimeStamp(input) {

    function computeSeconds(h, m, s, f) {
      return (h | 0) * 3600 + (m | 0) * 60 + (s | 0) + (f | 0) / 1000;
    }

    var m = input.match(/^(\d+):(\d{2})(:\d{2})?\.(\d{3})/);
    if (!m)
      return null;

    if (m[3]) {
      
      return computeSeconds(m[1], m[2], m[3].replace(":", ""), m[4]);
    } else if (m[1] > 59) {
      
      
      return computeSeconds(m[1], m[2], 0,  m[4]);
    } else {
      
      return computeSeconds(0, m[1], m[2], m[4]);
    }
  }

  
  
  function Settings() {
    this.values = Object.create(null);
  }

  Settings.prototype = {
    
    set: function(k, v) {
      if (!this.get(k) && v !== "")
        this.values[k] = v;
    },
    
    get: function(k, dflt) {
      return this.has(k) ? this.values[k] : dflt;
    },
    
    has: function(k) {
      return k in this.values;
    },
    
    alt: function(k, v, a) {
      for (var n = 0; n < a.length; ++n) {
        if (v === a[n]) {
          this.set(k, v);
          break;
        }
      }
    },
    
    region: function(k, v) {
      if (!v.match(/-->/)) {
        this.set(k, v);
      }
    },
    
    integer: function(k, v) {
      if (/^-?\d+$/.test(v)) 
        this.set(k, parseInt(v, 10));
    },
    
    percent: function(k, v, frac) {
      var m;
      if ((m = v.match(/^([\d]{1,3})(\.[\d]*)?%$/))) {
        v = v.replace("%", "");
        if (!m[2] || (m[2] && frac)) {
          v = parseFloat(v);
          if (v >= 0 && v <= 100) {
            this.set(k, v);
            return true;
          }
        }
      }
      return false;
    }
  };

  
  
  function parseOptions(input, callback, keyValueDelim, groupDelim) {
    var groups = groupDelim ? input.split(groupDelim) : [input];
    for (var i in groups) {
      var kv = groups[i].split(keyValueDelim);
      if (kv.length !== 2)
        continue;
      var k = kv[0];
      var v = kv[1];
      callback(k, v);
    }
  }

  function parseCue(input, cue) {
    
    function consumeTimeStamp() {
      var ts = parseTimeStamp(input);
      if (ts === null)
        throw "error";
      
      input = input.replace(/^[^\s-]+/, "");
      return ts;
    }

    
    function consumeCueSettings(input, cue) {
      var settings = new Settings();

      parseOptions(input, function (k, v) {
        switch (k) {
        case "region":
          settings.region(k, v);
          break;
        case "vertical":
          settings.alt(k, v, ["rl", "lr"]);
          break;
        case "line":
          settings.integer(k, v);
          settings.percent(k, v) ? settings.set("snapToLines", false) : null;
          settings.alt(k, v, ["auto"]);
          break;
        case "position":
        case "size":
          settings.percent(k, v);
          break;
        case "align":
          settings.alt(k, v, ["start", "middle", "end", "left", "right"]);
          break;
        }
      }, /:/, /\s/);

      
      cue.regionId = settings.get("region", "");
      cue.vertical = settings.get("vertical", "");
      cue.line = settings.get("line", "auto");
      cue.snapToLines = settings.get("snapToLines", true);
      cue.position = settings.get("position", 50);
      cue.size = settings.get("size", 100);
      cue.align = settings.get("align", "middle");
    }

    function skipWhitespace() {
      input = input.replace(/^\s+/, "");
    }

    
    skipWhitespace();
    cue.startTime = consumeTimeStamp();   
    skipWhitespace();
    if (input.substr(0, 3) !== "-->")     
      throw "error";
    input = input.substr(3);
    skipWhitespace();
    cue.endTime = consumeTimeStamp();     

    
    skipWhitespace();
    consumeCueSettings(input, cue);
  }

  const ESCAPE = {
    "&amp;": "&",
    "&lt;": "<",
    "&gt;": ">",
    "&lrm;": "\u200e",
    "&rlm;": "\u200f",
    "&nbsp;": "\u00a0"
  };

  const TAG_NAME = {
    c: "span",
    i: "i",
    b: "b",
    u: "u",
    ruby: "ruby",
    rt: "rt",
    v: "span",
    lang: "span"
  };

  const TAG_ANNOTATION = {
    v: "title",
    lang: "lang"
  };

  const NEEDS_PARENT = {
    rt: "ruby"
  };

  
  function parseContent(window, input) {
    function nextToken() {
      
      if (!input)
        return null;

      
      function consume(result) {
        input = input.substr(result.length);
        return result;
      }

      var m = input.match(/^([^<]*)(<[^>]+>?)?/);
      
      
      return consume(m[1] ? m[1] : m[2]);
    }

    
    function unescape1(e) {
      return ESCAPE[e];
    }
    function unescape(s) {
      while ((m = s.match(/&(amp|lt|gt|lrm|rlm|nbsp);/)))
        s = s.replace(m[0], unescape1);
      return s;
    }

    function shouldAdd(current, element) {
      return !NEEDS_PARENT[element.localName] ||
             NEEDS_PARENT[element.localName] === current.localName;
    }

    
    function createElement(type, annotation) {
      var tagName = TAG_NAME[type];
      if (!tagName)
        return null;
      var element = window.document.createElement(tagName);
      element.localName = tagName;
      var name = TAG_ANNOTATION[type];
      if (name && annotation)
        element[name] = annotation.trim();
      return element;
    }

    var rootDiv = window.document.createElement("div"),
        current = rootDiv,
        t,
        tagStack = [];

    while ((t = nextToken()) !== null) {
      if (t[0] === '<') {
        if (t[1] === "/") {
          
          if (tagStack.length &&
              tagStack[tagStack.length - 1] === t.substr(2).replace(">", "")) {
            tagStack.pop();
            current = current.parentNode;
          }
          
          continue;
        }
        var ts = parseTimeStamp(t.substr(1, t.length - 2));
        var node;
        if (ts) {
          
          node = window.ProcessingInstruction();
          node.target = "timestamp";
          node.data = ts;
          current.appendChild(node);
          continue;
        }
        var m = t.match(/^<([^.\s/0-9>]+)(\.[^\s\\>]+)?([^>\\]+)?(\\?)>?$/);
        
        if (!m)
          continue;
        
        node = createElement(m[1], m[3]);
        if (!node)
          continue;
        
        
        if (!shouldAdd(current, node))
          continue;
        
        if (m[2])
          node.className = m[2].substr(1).replace('.', ' ');
        
        
        tagStack.push(m[1]);
        current.appendChild(node);
        current = node;
        continue;
      }

      
      current.appendChild(window.document.createTextNode(unescape(t)));
    }

    return rootDiv;
  }

  function computeLinePos(cue) {
    if (typeof cue.line === "number" &&
        (cue.snapToLines || (cue.line >= 0 && cue.line <= 100)))
      return cue.line;
    if (!cue.track)
      return -1;
    
    
    
    return 100;
  }

  function CueBoundingBox(cue) {
    
    this.direction = "ltr";

    var boxLen = (function(direction){
      var maxLen = 0;
      if ((cue.vertical === "" &&
          (cue.align === "left" ||
           (cue.align === "start" && direction === "ltr") ||
           (cue.align === "end" && direction === "rtl"))) ||
         ((cue.vertical === "rl" || cue.vertical === "lr") &&
          (cue.align === "start" || cue.align === "left")))
        maxLen = 100 - cue.position;
      else if ((cue.vertical === "" &&
                (cue.align === "right" ||
                 (cue.align === "end" && direction === "ltr") ||
                 (cue.align === "start" && direction === "rtl"))) ||
               ((cue.vertical === "rl" || cue.vertical === "lr") &&
                 (cue.align === "end" || cue.align === "right")))
        maxLen = cue.position;
      else if (cue.align === "middle") {
        if (cue.position <= 50)
          maxLen = cue.position * 2;
        else
          maxLen = (100 - cue.position) * 2;
      }
      return cue.size < maxLen ? cue.size : maxLen;
    }(this.direction));

    this.left = (function(direction) {
      if (cue.vertical === "") {
        if (direction === "ltr") {
          if (cue.align === "start" || cue.align === "left")
            return cue.position;
          else if (cue.align === "end" || cue.align === "right")
            return cue.position - boxLen;
          else if (cue.align === "middle")
            return cue.position - (boxLen / 2);
        } else if (direction === "rtl") {
          if (cue.align === "end" || cue.align === "left")
            return 100 - cue.position;
          else if (cue.align === "start" || cue.align === "right")
            return 100 - cue.position - boxLen;
          else if (cue.align === "middle")
            return 100 - cue.position - (boxLen / 2);
        }
      }
      return cue.snapToLines ? 0 : computeLinePos(cue);
    }(this.direction));

    this.top = (function() {
      if (cue.vertical === "rl" || cue.vertical === "lr") {
        if (cue.align === "start" || cue.align === "left")
          return cue.position;
        else if (cue.align === "end" || cue.align === "right")
          return cue.position - boxLen;
        else if (cue.align === "middle")
          return cue.position - (boxLen / 2);
      }
      return cue.snapToLines ? 0 : computeLinePos(cue);
    }());

    
    
    var edgeMargin = 10;
    if (cue.snapToLines) {
      if (cue.vertical === "") {
        if (this.left < edgeMargin && this.left + boxLen > edgeMargin) {
          this.left += edgeMargin;
          boxLen -= edgeMargin;
        }
        var rightMargin = 100 - edgeMargin;
        if (this.left < rightMargin && this.left + boxLen > rightMargin)
          boxLen -= edgeMargin;
      } else if (cue.vertical === "lr" || cue.vertical === "rl") {
        if (this.top < edgeMargin && this.top + boxLen > edgeMargin) {
          this.top += edgeMargin;
          boxLen -= edgeMargin;
        }
        var bottomMargin = 100 - edgeMargin;
        if (this.top < bottomMargin && this.top + boxLen > bottomMargin)
          boxLen -= edgeMargin;
      }
    }

    this.height = cue.vertical === "" ? "auto" : boxLen;
    this.width = cue.vertical === "" ? boxLen : "auto";

    this.writingMode = cue.vertical === "" ?
                       "horizontal-tb" :
                       cue.vertical === "lr" ? "vertical-lr" : "vertical-rl";
    this.position = "absolute";
    this.unicodeBidi = "plaintext";
    this.textAlign = cue.align === "middle" ? "center" : cue.align;
    this.font = "5vh sans-serif";
    this.color = "rgba(255,255,255,1)";
    this.whiteSpace = "pre-line";
  }

  const WEBVTT = "WEBVTT";

  function WebVTTParser(window, decoder) {
    this.window = window;
    this.state = "INITIAL";
    this.buffer = "";
    this.decoder = decoder || TextDecoder("utf8");
  }

  
  WebVTTParser.StringDecoder = function() {
    return {
      decode: function(data) {
        if (!data) return "";
        if (typeof data !== "string") throw "[StringDecoder] Error - expected string data";

        return decodeURIComponent(escape(data));
      }
    };
  };

  WebVTTParser.convertCueToDOMTree = function(window, cuetext) {
    if (!window || !cuetext)
      return null;
    return parseContent(window, cuetext);
  };

  WebVTTParser.processCues = function(window, cues) {
    if (!window || !cues)
      return null;

    return cues.map(function(cue) {
      var div = parseContent(window, cue.text);
      div.style = new CueBoundingBox(cue);
      
      
      return div;
    });
  };

  WebVTTParser.prototype = {
    parse: function (data) {
      var self = this;

      
      
      
      if (data) {
        
        self.buffer += self.decoder.decode(data, {stream: true});
      }

      
      
      function collectNextLine(advance) {
        var buffer = self.buffer;
        var pos = 0;
        advance = typeof advance === "undefined" ? true : advance;
        while (pos < buffer.length && buffer[pos] != '\r' && buffer[pos] != '\n')
          ++pos;
        var line = buffer.substr(0, pos);
        
        if (buffer[pos] === '\r')
          ++pos;
        if (buffer[pos] === '\n')
          ++pos;
        if (advance)
          self.buffer = buffer.substr(pos);
        return line;
      }

      
      function parseRegion(input) {
        var settings = new Settings();

        parseOptions(input, function (k, v) {
          switch (k) {
          case "id":
            settings.region(k, v);
            break;
          case "width":
            settings.percent(k, v, true);
            break;
          case "lines":
            settings.integer(k, v);
            break;
          case "regionanchor":
          case "viewportanchor":
            var xy = v.split(',');
            if (xy.length !== 2)
              break;
            
            
            var anchor = new Settings();
            anchor.percent("x", xy[0], true);
            anchor.percent("y", xy[1], true);
            if (!anchor.has("x") || !anchor.has("y"))
              break;
            settings.set(k + "X", anchor.get("x"));
            settings.set(k + "Y", anchor.get("y"));
            break;
          case "scroll":
            settings.alt(k, v, ["up"]);
            break;
          }
        }, /=/, /\s/);

        
        
        if (self.onregion && settings.has("id")) {
          var region = new self.window.VTTRegion();
          region.id = settings.get("id");
          region.width = settings.get("width", 100);
          region.lines = settings.get("lines", 3);
          region.regionAnchorX = settings.get("regionanchorX", 0);
          region.regionAnchorY = settings.get("regionanchorY", 100);
          region.viewportAnchorX = settings.get("viewportanchorX", 0);
          region.viewportAnchorY = settings.get("viewportanchorY", 100);
          region.scroll = settings.get("scroll", "none");
          self.onregion(region);
        }
      }

      
      function parseHeader(input) {
        parseOptions(input, function (k, v) {
          switch (k) {
          case "Region":
            
            parseRegion(v);
            break;
          }
        }, /:/);
      }

      
      try {
        var line;
        if (self.state === "INITIAL") {
          
          if (self.buffer.length <= WEBVTT.length)
            return this;

          
          
          
          line = collectNextLine(false);
          
          
          if (line.substr(0, WEBVTT.length) !== WEBVTT ||
              line.length > WEBVTT.length && !/[ \t]/.test(line[WEBVTT.length])) {
            throw "error";
          }
          
          
          collectNextLine(true);
          self.state = "HEADER";
        }

        while (self.buffer) {
          
          if (!/[\r\n]/.test(self.buffer)) {
            
            
            if (self.state === "CUETEXT" && self.cue && self.onpartialcue)
              self.onpartialcue(self.cue);
            return this;
          }

          line = collectNextLine();

          switch (self.state) {
          case "HEADER":
            
            if (/:/.test(line)) {
              parseHeader(line);
            } else if (!line) {
              
              self.state = "ID";
            }
            continue;
          case "NOTE":
            
            if (!line)
              self.state = "ID";
            continue;
          case "ID":
            
            if (/^NOTE($|[ \t])/.test(line)) {
              self.state = "NOTE";
              break;
            }
            
            if (!line)
              continue;
            self.cue = new self.window.VTTCue(0, 0, "");
            self.state = "CUE";
            
            if (line.indexOf("-->") == -1) {
              self.cue.id = line;
              continue;
            }
            
            
          case "CUE":
            
            try {
              parseCue(line, self.cue);
            } catch (e) {
              
              self.cue = null;
              self.state = "BADCUE";
              continue;
            }
            self.state = "CUETEXT";
            continue;
          case "CUETEXT":
            
            if (!line) {
              
              self.oncue && self.oncue(self.cue);
              self.cue = null;
              self.state = "ID";
              continue;
            }
            if (self.cue.text)
              self.cue.text += "\n";
            self.cue.text += line;
            continue;
          default: 
            
            if (!line) {
              self.state = "ID";
            }
            continue;
          }
        }
      } catch (e) {
        
        if (self.state === "CUETEXT" && self.cue && self.oncue)
          self.oncue(self.cue);
        self.cue = null;
        
        
        if (self.state !== "INITIAL")
          self.state = "BADCUE";
      }
      return this;
    },
    flush: function () {
      var self = this;
      
      self.buffer += self.decoder.decode();
      
      if (self.cue || self.state === "HEADER") {
        self.buffer += "\n\n";
        self.parse();
      }
      self.onflush && self.onflush();
      return this;
    }
  };

  global.WebVTTParser = WebVTTParser;

}(this));
