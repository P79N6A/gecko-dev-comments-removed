




function setBody(data) {
  $("#legal-copy").html(data);
}

function normalizeLocale(lang) {
  return lang.replace(/-/g, "_");
}

$(document).ready(function() {
  
  var lang, defaultLang = "en-US";
  $.get(loop.config.serverUrl, function(data) {
    if (data.hasOwnProperty("i18n")) {
      lang = normalizeLocale(data.i18n.lang);
      defaultLang = normalizeLocale(data.i18n.defaultLang);
    }
    if (lang === undefined) {
      lang = normalizeLocale(defaultLang);
    }

    $.get(lang + ".html")
      .done(setBody)
      .fail(function() {
        $.get(defaultLang + ".html")
          .done(setBody);
      });
  });
});
