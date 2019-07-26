








var defaultLocale = new Intl.NumberFormat().resolvedOptions().locale;
var notSupported = 'zxx'; 
var requestedLocales = [defaultLocale, notSupported];
    
var supportedLocales;

if (!Intl.NumberFormat.hasOwnProperty('supportedLocalesOf')) {
    $ERROR("Intl.NumberFormat doesn't have a supportedLocalesOf property.");
}
    
supportedLocales = Intl.NumberFormat.supportedLocalesOf(requestedLocales);
if (supportedLocales.length !== 1) {
    $ERROR('The length of supported locales list is not 1.');
}
    
if (supportedLocales[0] !== defaultLocale) {
    $ERROR('The default locale is not returned in the supported list.');
}

