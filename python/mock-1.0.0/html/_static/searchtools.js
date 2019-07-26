


















jQuery.makeSearchSummary = function(text, keywords, hlwords) {
  var textLower = text.toLowerCase();
  var start = 0;
  $.each(keywords, function() {
    var i = textLower.indexOf(this.toLowerCase());
    if (i > -1)
      start = i;
  });
  start = Math.max(start - 120, 0);
  var excerpt = ((start > 0) ? '...' : '') +
  $.trim(text.substr(start, 240)) +
  ((start + 240 - text.length) ? '...' : '');
  var rv = $('<div class="context"></div>').text(excerpt);
  $.each(hlwords, function() {
    rv = rv.highlightText(this, 'highlighted');
  });
  return rv;
}





var Stemmer = function() {

  var step2list = {
    ational: 'ate',
    tional: 'tion',
    enci: 'ence',
    anci: 'ance',
    izer: 'ize',
    bli: 'ble',
    alli: 'al',
    entli: 'ent',
    eli: 'e',
    ousli: 'ous',
    ization: 'ize',
    ation: 'ate',
    ator: 'ate',
    alism: 'al',
    iveness: 'ive',
    fulness: 'ful',
    ousness: 'ous',
    aliti: 'al',
    iviti: 'ive',
    biliti: 'ble',
    logi: 'log'
  };

  var step3list = {
    icate: 'ic',
    ative: '',
    alize: 'al',
    iciti: 'ic',
    ical: 'ic',
    ful: '',
    ness: ''
  };

  var c = "[^aeiou]";          
  var v = "[aeiouy]";          
  var C = c + "[^aeiouy]*";    
  var V = v + "[aeiou]*";      

  var mgr0 = "^(" + C + ")?" + V + C;                      
  var meq1 = "^(" + C + ")?" + V + C + "(" + V + ")?$";    
  var mgr1 = "^(" + C + ")?" + V + C + V + C;              
  var s_v   = "^(" + C + ")?" + v;                         

  this.stemWord = function (w) {
    var stem;
    var suffix;
    var firstch;
    var origword = w;

    if (w.length < 3)
      return w;

    var re;
    var re2;
    var re3;
    var re4;

    firstch = w.substr(0,1);
    if (firstch == "y")
      w = firstch.toUpperCase() + w.substr(1);

    
    re = /^(.+?)(ss|i)es$/;
    re2 = /^(.+?)([^s])s$/;

    if (re.test(w))
      w = w.replace(re,"$1$2");
    else if (re2.test(w))
      w = w.replace(re2,"$1$2");

    
    re = /^(.+?)eed$/;
    re2 = /^(.+?)(ed|ing)$/;
    if (re.test(w)) {
      var fp = re.exec(w);
      re = new RegExp(mgr0);
      if (re.test(fp[1])) {
        re = /.$/;
        w = w.replace(re,"");
      }
    }
    else if (re2.test(w)) {
      var fp = re2.exec(w);
      stem = fp[1];
      re2 = new RegExp(s_v);
      if (re2.test(stem)) {
        w = stem;
        re2 = /(at|bl|iz)$/;
        re3 = new RegExp("([^aeiouylsz])\\1$");
        re4 = new RegExp("^" + C + v + "[^aeiouwxy]$");
        if (re2.test(w))
          w = w + "e";
        else if (re3.test(w)) {
          re = /.$/;
          w = w.replace(re,"");
        }
        else if (re4.test(w))
          w = w + "e";
      }
    }

    
    re = /^(.+?)y$/;
    if (re.test(w)) {
      var fp = re.exec(w);
      stem = fp[1];
      re = new RegExp(s_v);
      if (re.test(stem))
        w = stem + "i";
    }

    
    re = /^(.+?)(ational|tional|enci|anci|izer|bli|alli|entli|eli|ousli|ization|ation|ator|alism|iveness|fulness|ousness|aliti|iviti|biliti|logi)$/;
    if (re.test(w)) {
      var fp = re.exec(w);
      stem = fp[1];
      suffix = fp[2];
      re = new RegExp(mgr0);
      if (re.test(stem))
        w = stem + step2list[suffix];
    }

    
    re = /^(.+?)(icate|ative|alize|iciti|ical|ful|ness)$/;
    if (re.test(w)) {
      var fp = re.exec(w);
      stem = fp[1];
      suffix = fp[2];
      re = new RegExp(mgr0);
      if (re.test(stem))
        w = stem + step3list[suffix];
    }

    
    re = /^(.+?)(al|ance|ence|er|ic|able|ible|ant|ement|ment|ent|ou|ism|ate|iti|ous|ive|ize)$/;
    re2 = /^(.+?)(s|t)(ion)$/;
    if (re.test(w)) {
      var fp = re.exec(w);
      stem = fp[1];
      re = new RegExp(mgr1);
      if (re.test(stem))
        w = stem;
    }
    else if (re2.test(w)) {
      var fp = re2.exec(w);
      stem = fp[1] + fp[2];
      re2 = new RegExp(mgr1);
      if (re2.test(stem))
        w = stem;
    }

    
    re = /^(.+?)e$/;
    if (re.test(w)) {
      var fp = re.exec(w);
      stem = fp[1];
      re = new RegExp(mgr1);
      re2 = new RegExp(meq1);
      re3 = new RegExp("^" + C + v + "[^aeiouwxy]$");
      if (re.test(stem) || (re2.test(stem) && !(re3.test(stem))))
        w = stem;
    }
    re = /ll$/;
    re2 = new RegExp(mgr1);
    if (re.test(w) && re2.test(w)) {
      re = /.$/;
      w = w.replace(re,"");
    }

    
    if (firstch == "y")
      w = firstch.toLowerCase() + w.substr(1);
    return w;
  }
}





var Search = {

  _index : null,
  _queued_query : null,
  _pulse_status : -1,

  init : function() {
      var params = $.getQueryParameters();
      if (params.q) {
          var query = params.q[0];
          $('input[name="q"]')[0].value = query;
          this.performSearch(query);
      }
  },

  loadIndex : function(url) {
    $.ajax({type: "GET", url: url, data: null, success: null,
            dataType: "script", cache: true});
  },

  setIndex : function(index) {
    var q;
    this._index = index;
    if ((q = this._queued_query) !== null) {
      this._queued_query = null;
      Search.query(q);
    }
  },

  hasIndex : function() {
      return this._index !== null;
  },

  deferQuery : function(query) {
      this._queued_query = query;
  },

  stopPulse : function() {
      this._pulse_status = 0;
  },

  startPulse : function() {
    if (this._pulse_status >= 0)
        return;
    function pulse() {
      Search._pulse_status = (Search._pulse_status + 1) % 4;
      var dotString = '';
      for (var i = 0; i < Search._pulse_status; i++)
        dotString += '.';
      Search.dots.text(dotString);
      if (Search._pulse_status > -1)
        window.setTimeout(pulse, 500);
    };
    pulse();
  },

  


  performSearch : function(query) {
    
    this.out = $('#search-results');
    this.title = $('<h2>' + _('Searching') + '</h2>').appendTo(this.out);
    this.dots = $('<span></span>').appendTo(this.title);
    this.status = $('<p style="display: none"></p>').appendTo(this.out);
    this.output = $('<ul class="search"/>').appendTo(this.out);

    $('#search-progress').text(_('Preparing search...'));
    this.startPulse();

    
    if (this.hasIndex())
      this.query(query);
    else
      this.deferQuery(query);
  },

  query : function(query) {
    var stopwords = ["and","then","into","it","as","are","in","if","for","no","there","their","was","is","be","to","that","but","they","not","such","with","by","a","on","these","of","will","this","near","the","or","at"];

    
    var stemmer = new Stemmer();
    var searchterms = [];
    var excluded = [];
    var hlterms = [];
    var tmp = query.split(/\s+/);
    var objectterms = [];
    for (var i = 0; i < tmp.length; i++) {
      if (tmp[i] != "") {
          objectterms.push(tmp[i].toLowerCase());
      }

      if ($u.indexOf(stopwords, tmp[i]) != -1 || tmp[i].match(/^\d+$/) ||
          tmp[i] == "") {
        
        continue;
      }
      
      var word = stemmer.stemWord(tmp[i]).toLowerCase();
      
      if (word[0] == '-') {
        var toAppend = excluded;
        word = word.substr(1);
      }
      else {
        var toAppend = searchterms;
        hlterms.push(tmp[i].toLowerCase());
      }
      
      if (!$.contains(toAppend, word))
        toAppend.push(word);
    };
    var highlightstring = '?highlight=' + $.urlencode(hlterms.join(" "));

    
    
    

    
    var filenames = this._index.filenames;
    var titles = this._index.titles;
    var terms = this._index.terms;
    var fileMap = {};
    var files = null;
    
    var importantResults = [];
    var objectResults = [];
    var regularResults = [];
    var unimportantResults = [];
    $('#search-progress').empty();

    
    for (var i = 0; i < objectterms.length; i++) {
      var others = [].concat(objectterms.slice(0,i),
                             objectterms.slice(i+1, objectterms.length))
      var results = this.performObjectSearch(objectterms[i], others);
      
      
      
      
      objectResults = results[0].concat(objectResults);
      importantResults = results[1].concat(importantResults);
      unimportantResults = results[2].concat(unimportantResults);
    }

    
    for (var i = 0; i < searchterms.length; i++) {
      var word = searchterms[i];
      
      if ((files = terms[word]) == null)
        break;
      if (files.length == undefined) {
        files = [files];
      }
      
      for (var j = 0; j < files.length; j++) {
        var file = files[j];
        if (file in fileMap)
          fileMap[file].push(word);
        else
          fileMap[file] = [word];
      }
    }

    
    for (var file in fileMap) {
      var valid = true;

      
      if (fileMap[file].length != searchterms.length)
        continue;

      
      
      for (var i = 0; i < excluded.length; i++) {
        if (terms[excluded[i]] == file ||
            $.contains(terms[excluded[i]] || [], file)) {
          valid = false;
          break;
        }
      }

      
      
      if (valid)
        regularResults.push([filenames[file], titles[file], '', null]);
    }

    
    
    delete filenames, titles, terms;

    
    regularResults.sort(function(a, b) {
      var left = a[1].toLowerCase();
      var right = b[1].toLowerCase();
      return (left > right) ? -1 : ((left < right) ? 1 : 0);
    });

    
    var results = unimportantResults.concat(regularResults)
      .concat(objectResults).concat(importantResults);

    
    var resultCount = results.length;
    function displayNextItem() {
      
      if (results.length) {
        var item = results.pop();
        var listItem = $('<li style="display:none"></li>');
        if (DOCUMENTATION_OPTIONS.FILE_SUFFIX == '') {
          
          var dirname = item[0] + '/';
          if (dirname.match(/\/index\/$/)) {
            dirname = dirname.substring(0, dirname.length-6);
          } else if (dirname == 'index/') {
            dirname = '';
          }
          listItem.append($('<a/>').attr('href',
            DOCUMENTATION_OPTIONS.URL_ROOT + dirname +
            highlightstring + item[2]).html(item[1]));
        } else {
          
          listItem.append($('<a/>').attr('href',
            item[0] + DOCUMENTATION_OPTIONS.FILE_SUFFIX +
            highlightstring + item[2]).html(item[1]));
        }
        if (item[3]) {
          listItem.append($('<span> (' + item[3] + ')</span>'));
          Search.output.append(listItem);
          listItem.slideDown(5, function() {
            displayNextItem();
          });
        } else if (DOCUMENTATION_OPTIONS.HAS_SOURCE) {
          $.get(DOCUMENTATION_OPTIONS.URL_ROOT + '_sources/' +
                item[0] + '.txt', function(data) {
            if (data != '') {
              listItem.append($.makeSearchSummary(data, searchterms, hlterms));
              Search.output.append(listItem);
            }
            listItem.slideDown(5, function() {
              displayNextItem();
            });
          }, "text");
        } else {
          
          Search.output.append(listItem);
          listItem.slideDown(5, function() {
            displayNextItem();
          });
        }
      }
      
      else {
        Search.stopPulse();
        Search.title.text(_('Search Results'));
        if (!resultCount)
          Search.status.text(_('Your search did not match any documents. Please make sure that all words are spelled correctly and that you\'ve selected enough categories.'));
        else
            Search.status.text(_('Search finished, found %s page(s) matching the search query.').replace('%s', resultCount));
        Search.status.fadeIn(500);
      }
    }
    displayNextItem();
  },

  performObjectSearch : function(object, otherterms) {
    var filenames = this._index.filenames;
    var objects = this._index.objects;
    var objnames = this._index.objnames;
    var titles = this._index.titles;

    var importantResults = [];
    var objectResults = [];
    var unimportantResults = [];

    for (var prefix in objects) {
      for (var name in objects[prefix]) {
        var fullname = (prefix ? prefix + '.' : '') + name;
        if (fullname.toLowerCase().indexOf(object) > -1) {
          var match = objects[prefix][name];
          var objname = objnames[match[1]][2];
          var title = titles[match[0]];
          
          
          if (otherterms.length > 0) {
            var haystack = (prefix + ' ' + name + ' ' +
                            objname + ' ' + title).toLowerCase();
            var allfound = true;
            for (var i = 0; i < otherterms.length; i++) {
              if (haystack.indexOf(otherterms[i]) == -1) {
                allfound = false;
                break;
              }
            }
            if (!allfound) {
              continue;
            }
          }
          var descr = objname + _(', in ') + title;
          anchor = match[3];
          if (anchor == '')
            anchor = fullname;
          else if (anchor == '-')
            anchor = objnames[match[1]][1] + '-' + fullname;
          result = [filenames[match[0]], fullname, '#'+anchor, descr];
          switch (match[2]) {
          case 1: objectResults.push(result); break;
          case 0: importantResults.push(result); break;
          case 2: unimportantResults.push(result); break;
          }
        }
      }
    }

    
    objectResults.sort(function(a, b) {
      return (a[1] > b[1]) ? -1 : ((a[1] < b[1]) ? 1 : 0);
    });

    importantResults.sort(function(a, b) {
      return (a[1] > b[1]) ? -1 : ((a[1] < b[1]) ? 1 : 0);
    });

    unimportantResults.sort(function(a, b) {
      return (a[1] > b[1]) ? -1 : ((a[1] < b[1]) ? 1 : 0);
    });

    return [importantResults, objectResults, unimportantResults]
  }
}

$(document).ready(function() {
  Search.init();
});