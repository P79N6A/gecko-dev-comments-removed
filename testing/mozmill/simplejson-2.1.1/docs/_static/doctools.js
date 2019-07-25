





if (!window.console || !console.firebug) {
  var names = ["log", "debug", "info", "warn", "error", "assert", "dir", "dirxml",
      "group", "groupEnd", "time", "timeEnd", "count", "trace", "profile", "profileEnd"];
  window.console = {};
  for (var i = 0; i < names.length; ++i)
    window.console[names[i]] = function() {}
}




jQuery.urldecode = function(x) {
  return decodeURIComponent(x).replace(/\+/g, ' ');
}




jQuery.urlencode = encodeURIComponent;






jQuery.getQueryParameters = function(s) {
  if (typeof s == 'undefined')
    s = document.location.search;
  var parts = s.substr(s.indexOf('?') + 1).split('&');
  var result = {};
  for (var i = 0; i < parts.length; i++) {
    var tmp = parts[i].split('=', 2);
    var key = jQuery.urldecode(tmp[0]);
    var value = jQuery.urldecode(tmp[1]);
    if (key in result)
      result[key].push(value);
    else
      result[key] = [value];
  }
  return result;
}





jQuery.contains = function(arr, item) {
  for (var i = 0; i < arr.length; i++) {
    if (arr[i] == item)
      return true;
  }
  return false;
}





jQuery.fn.highlightText = function(text, className) {
  function highlight(node) {
    if (node.nodeType == 3) {
      var val = node.nodeValue;
      var pos = val.toLowerCase().indexOf(text);
      if (pos >= 0 && !jQuery.className.has(node.parentNode, className)) {
        var span = document.createElement("span");
        span.className = className;
        span.appendChild(document.createTextNode(val.substr(pos, text.length)));
        node.parentNode.insertBefore(span, node.parentNode.insertBefore(
          document.createTextNode(val.substr(pos + text.length)),
          node.nextSibling));
        node.nodeValue = val.substr(0, pos);
      }
    }
    else if (!jQuery(node).is("button, select, textarea")) {
      jQuery.each(node.childNodes, function() {
        highlight(this)
      });
    }
  }
  return this.each(function() {
    highlight(this);
  });
}




var Documentation = {

  init : function() {
    
    this.fixFirefoxAnchorBug();
    this.highlightSearchWords();
    this.initModIndex();
    this.initComments();
  },

  


  addContextElements : function() {
    for (var i = 1; i <= 6; i++) {
      $('h' + i + '[@id]').each(function() {
        $('<a class="headerlink">\u00B6</a>').
        attr('href', '#' + this.id).
        attr('title', 'Permalink to this headline').
        appendTo(this);
      });
    }
    $('dt[@id]').each(function() {
      $('<a class="headerlink">\u00B6</a>').
      attr('href', '#' + this.id).
      attr('title', 'Permalink to this definition').
      appendTo(this);
    });
  },

  


  fixFirefoxAnchorBug : function() {
    if (document.location.hash && $.browser.mozilla)
      window.setTimeout(function() {
        document.location.href += '';
      }, 10);
  },

  


  highlightSearchWords : function() {
    var params = $.getQueryParameters();
    var terms = (params.highlight) ? params.highlight[0].split(/\s+/) : [];
    if (terms.length) {
      var body = $('div.body');
      window.setTimeout(function() {
        $.each(terms, function() {
          body.highlightText(this.toLowerCase(), 'highlight');
        });
      }, 10);
      $('<li class="highlight-link"><a href="javascript:Documentation.' +
        'hideSearchWords()">Hide Search Matches</a></li>')
          .appendTo($('.sidebar .this-page-menu'));
    }
  },

  


  initModIndex : function() {
    var togglers = $('img.toggler').click(function() {
      var src = $(this).attr('src');
      var idnum = $(this).attr('id').substr(7);
      console.log($('tr.cg-' + idnum).toggle());
      if (src.substr(-9) == 'minus.png')
        $(this).attr('src', src.substr(0, src.length-9) + 'plus.png');
      else
        $(this).attr('src', src.substr(0, src.length-8) + 'minus.png');
    }).css('display', '');
    if (DOCUMENTATION_OPTIONS.COLLAPSE_MODINDEX) {
        togglers.click();
    }
  },

  


  initComments : function() {
    $('.inlinecomments div.actions').each(function() {
      this.innerHTML += ' | ';
      $(this).append($('<a href="#">hide comments</a>').click(function() {
        $(this).parent().parent().toggle();
        return false;
      }));
    });
    $('.inlinecomments .comments').hide();
    $('.inlinecomments a.bubble').each(function() {
      $(this).click($(this).is('.emptybubble') ? function() {
          var params = $.getQueryParameters(this.href);
          Documentation.newComment(params.target[0]);
          return false;
        } : function() {
          $('.comments', $(this).parent().parent()[0]).toggle();
          return false;
      });
    });
    $('#comments div.actions a.newcomment').click(function() {
      Documentation.newComment();
      return false;
    });
    if (document.location.hash.match(/^#comment-/))
      $('.inlinecomments .comments ' + document.location.hash)
        .parent().toggle();
  },

  


  hideSearchWords : function() {
    $('.sidebar .this-page-menu li.highlight-link').fadeOut(300);
    $('span.highlight').removeClass('highlight');
  },

  


  newComment : function(id) {
    Documentation.CommentWindow.openFor(id || '');
  },

  


  newCommentFromBox : function(link) {
    var params = $.getQueryParameters(link.href);
    $(link).parent().parent().fadeOut('slow');
    this.newComment(params.target);
  },

  


  makeURL : function(relativeURL) {
    return DOCUMENTATION_OPTIONS.URL_ROOT + '/' + relativeURL;
  },

  


  getCurrentURL : function() {
    var path = document.location.pathname;
    var parts = path.split(/\//);
    $.each(DOCUMENTATION_OPTIONS.URL_ROOT.split(/\//), function() {
      if (this == '..')
        parts.pop();
    });
    var url = parts.join('/');
    return path.substring(url.lastIndexOf('/') + 1, path.length - 1);
  },

  


  CommentWindow : (function() {
    var openWindows = {};

    var Window = function(sectionID) {
      this.url = Documentation.makeURL('@comments/' + Documentation.getCurrentURL()
        + '/?target=' + $.urlencode(sectionID) + '&mode=ajax');
      this.sectionID = sectionID;

      this.root = $('<div class="commentwindow"></div>');
      this.root.appendTo($('body'));
      this.title = $('<h3>New Comment</h3>').appendTo(this.root);
      this.body = $('<div class="form">please wait...</div>').appendTo(this.root);
      this.resizeHandle = $('<div class="resizehandle"></div>').appendTo(this.root);

      this.root.Draggable({
        handle:       this.title[0]
      });

      this.root.css({
        left:         window.innerWidth / 2 - $(this.root).width() / 2,
        top:          window.scrollY + (window.innerHeight / 2 - 150)
      });
      this.root.fadeIn('slow');
      this.updateView();
    };

    Window.prototype.updateView = function(data) {
      var self = this;
      function update(data) {
        if (data.posted) {
          document.location.hash = '#comment-' + data.commentID;
          document.location.reload();
        }
        else {
          self.body.html(data.body);
          $('div.actions', self.body).append($('<input>')
            .attr('type', 'button')
            .attr('value', 'Close')
            .click(function() { self.close(); })
          );
          $('div.actions input[@name="preview"]')
            .attr('type', 'button')
            .click(function() { self.submitForm($('form', self.body)[0], true); });
          $('form', self.body).bind("submit", function() {
            self.submitForm(this);
            return false;
          });

          if (data.error) {
            self.root.Highlight(1000, '#aadee1');
            $('div.error', self.root).slideDown(500);
          }
        }
      }

      if (typeof data == 'undefined')
        $.getJSON(this.url, function(json) { update(json); });
      else
        $.ajax({
          url:      this.url,
          type:     'POST',
          dataType: 'json',
          data:     data,
          success:  function(json) { update(json); }
        });
    }

    Window.prototype.getFormValue = function(name) {
      return $('*[@name="' + name + '"]', this.body)[0].value;
    }

    Window.prototype.submitForm = function(form, previewMode) {
      this.updateView({
        author:         form.author.value,
        author_mail:    form.author_mail.value,
        title:          form.title.value,
        comment_body:   form.comment_body.value,
        preview:        previewMode ? 'yes' : ''
      });
    }

    Window.prototype.close = function() {
      var self = this;
      delete openWindows[this.sectionID];
      this.root.fadeOut('slow', function() {
        self.root.remove();
      });
    }

    Window.openFor = function(sectionID) {
      if (sectionID in openWindows)
        return openWindows[sectionID];
      return new Window(sectionID);
    }

    return Window;
  })()
};


$(document).ready(function() {
  Documentation.init();
});
