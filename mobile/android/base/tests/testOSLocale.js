




const { utils: Cu, interfaces: Ci } = Components;

Cu.import("resource://gre/modules/Services.jsm");






function getOSLocale() {
  try {
    return Services.prefs.getCharPref("intl.locale.os");
  } catch (ex) {
    return null;
  }
}

function getAcceptLanguages() {
  return Services.prefs.getComplexValue("intl.accept_languages",
                                        Ci.nsIPrefLocalizedString).data;
}





add_test(function test_OSLocale() {
  let osObserver = function () {
    Services.prefs.removeObserver("intl.locale.os", osObserver);
    do_check_eq(getOSLocale(), "en-US");
    run_next_test();
  };

  let osLocale = getOSLocale();
  if (osLocale) {
    
    run_next_test();
    return;
  }

  
  
  Services.prefs.addObserver("intl.locale.os", osObserver, false);
});

add_test(function test_AcceptLanguages() {
  
  

  
  
  

  
  const ES_ES_EXPECTED_BASE = "es-ES,es,en-US,en";

  
  const MONO_EXPECTED_BASE = "en-US,en";

  
  
  const SELECTED_LOCALES = "es-es,fr,";

  
  const ES_ES_EXPECTED = SELECTED_LOCALES +
                         "es,en-us,en";      
  const MONO_EXPECTED = SELECTED_LOCALES +
                        "en-us,en";          

  let observer = function () {
    Services.prefs.removeObserver("intl.accept_languages", observer);

    
    let acc = getAcceptLanguages();
    let is_es = ES_ES_EXPECTED == acc;
    let is_en = MONO_EXPECTED == acc;

    do_check_true(is_es || is_en);
    do_check_true(is_es != is_en);
    run_next_test();
  };

  Services.prefs.setCharPref("intl.locale.os", "fr");
  Services.prefs.addObserver("intl.accept_languages", observer, false);
  Services.obs.notifyObservers(null, "Locale:Changed", "es-ES");
});

run_next_test();
