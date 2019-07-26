




package org.mozilla.gecko;

public class GeckoViewChrome implements GeckoView.ChromeDelegate {
    



    public void onReady(GeckoView view) {}

    







    public void onAlert(GeckoView view, GeckoView.Browser browser, String message, GeckoView.PromptResult result) {
        result.cancel();
    }

    







    public void onConfirm(GeckoView view, GeckoView.Browser browser, String message, GeckoView.PromptResult result) {
        result.cancel();
    }

    








    public void onPrompt(GeckoView view, GeckoView.Browser browser, String message, String defaultValue, GeckoView.PromptResult result) {
        result.cancel();
    }

    





    public void onDebugRequest(GeckoView view, GeckoView.PromptResult result) {
        result.cancel();
    }
}
