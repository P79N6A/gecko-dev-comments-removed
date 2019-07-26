









var extensions = ["-u-co-phonebk-kn", "-u-kn-co-phonebk"];
extensions.forEach(function (extension) {
    var defaultLocale = new Intl.Collator().resolvedOptions().locale;
    var collator = new Intl.Collator([defaultLocale + extension], {usage: "sort"});
    var locale = collator.resolvedOptions().locale;
    var numeric = collator.resolvedOptions().numeric;
    if (numeric !== undefined) {
        if (numeric !== true) {
            $ERROR("Default value for \"kn\" should be true, but is " + numeric + ".");
        }
        if (locale.indexOf("-kn") !== -1) {
            $ERROR("\"kn\" is returned in locale, but shouldn't be.");
        }
    }
});

