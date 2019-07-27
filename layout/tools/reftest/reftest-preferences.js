    
    
    
    
    branch.setBoolPref("dom.use_xbl_scopes_for_remote_xul", false);
    branch.setIntPref("gfx.color_management.mode", 2);
    branch.setBoolPref("gfx.color_management.force_srgb", true);
    branch.setBoolPref("browser.dom.window.dump.enabled", true);
    branch.setIntPref("ui.caretBlinkTime", -1);
    branch.setBoolPref("dom.send_after_paint_to_content", true);
    
    branch.setIntPref("dom.max_script_run_time", 0);
    branch.setIntPref("dom.max_chrome_script_run_time", 0);
    branch.setIntPref("hangmonitor.timeout", 0);
    
    branch.setBoolPref("media.autoplay.enabled", true);
    
    branch.setBoolPref("app.update.enabled", false);
    
    branch.setBoolPref("extensions.update.enabled", false);
    branch.setBoolPref("extensions.getAddons.cache.enabled", false);
    
    branch.setBoolPref("extensions.blocklist.enabled", false);
    
    branch.setIntPref("urlclassifier.updateinterval", 172800);
    
    branch.setBoolPref("image.high_quality_downscaling.enabled", false);
    
    
    
    branch.setBoolPref("image.single-color-optimization.enabled", false);
    
    
    branch.setBoolPref("security.fileuri.strict_origin_policy", false);
    
    branch.setBoolPref("browser.pagethumbnails.capturing_disabled", true);
    
    
    
    
    branch.setIntPref("browser.viewport.desktopWidth", 800);
    
    
    
    branch.setBoolPref("layout.testing.overlay-scrollbars.always-visible", true);
    
    
    
    
    
    
    
    branch.setBoolPref("layout.interruptible-reflow.enabled", false);
    
    
    branch.setIntPref("touchcaret.expiration.time", 0);

    
    
    branch.setBoolPref("browser.search.isUS", true);
    branch.setCharPref("browser.search.countryCode", "US");

    
    branch.setCharPref("browser.selfsupport.url", "https://%(server)s/selfsupport-dummy/");

    
    branch.setBoolPref("dom.serviceWorkers.periodic-updates.enabled", false);
