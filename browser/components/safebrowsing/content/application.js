




































var gDataProvider = null;















function PROT_Application() {
  this.debugZone= "application";

#ifdef DEBUG
  
  function runUnittests() {
    if (false) {

      G_DebugL("UNITTESTS", "STARTING UNITTESTS");
      TEST_G_Protocol4Parser();
      TEST_G_Base64();
      TEST_G_CryptoHasher();
      TEST_PROT_EnchashDecrypter();
      TEST_PROT_TRTable();
      TEST_PROT_ListManager();
      TEST_PROT_PhishingWarden();
      TEST_PROT_TRFetcher();
      TEST_G_ObjectSafeMap();
      TEST_PROT_URLCanonicalizer();
      TEST_G_Preferences();
      TEST_G_Observer();
      TEST_PROT_WireFormat();
      
      TEST_PROT_UrlCrypto();
      TEST_PROT_UrlCryptoKeyManager();
      G_DebugL("UNITTESTS", "END UNITTESTS");
    }
  };

  runUnittests();
#endif
  
  
  this.PROT_Controller = PROT_Controller;
  this.PROT_PhishingWarden = PROT_PhishingWarden;

  
  gDataProvider = new PROT_DataProvider();

  
  this.wrappedJSObject = this;
}





PROT_Application.prototype.getReportURL = function(name) {
  return gDataProvider["getReport" + name + "URL"]();
}
