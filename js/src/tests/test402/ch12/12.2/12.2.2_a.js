








var defaultLocale = new Intl.DateTimeFormat().resolvedOptions().locale;
var notSupported = 'zxx'; 
var requestedLocales = [defaultLocale, notSupported];
    
var supportedLocales;

if (!Intl.DateTimeFormat.hasOwnProperty('supportedLocalesOf')) {
    $ERROR("Intl.DateTimeFormat doesn't have a supportedLocalesOf property.");
}
    
supportedLocales = Intl.DateTimeFormat.supportedLocalesOf(requestedLocales);
if (supportedLocales.length !== 1) {
    $ERROR('The length of supported locales list is not 1.');
}
    
if (supportedLocales[0] !== defaultLocale) {
    $ERROR('The default locale is not returned in the supported list.');
}

