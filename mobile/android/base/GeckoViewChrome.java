




package org.mozilla.gecko;

public class GeckoViewChrome implements GeckoView.ChromeDelegate {
    



    @Override
    public void onReady(GeckoView view) {}

    







    @Override
    public void onAlert(GeckoView view, GeckoView.Browser browser, String message, GeckoView.PromptResult result) {
        result.cancel();
    }

    







    @Override
    public void onConfirm(GeckoView view, GeckoView.Browser browser, String message, GeckoView.PromptResult result) {
        result.cancel();
    }

    








    @Override
    public void onPrompt(GeckoView view, GeckoView.Browser browser, String message, String defaultValue, GeckoView.PromptResult result) {
        result.cancel();
    }

    





    @Override
    public void onDebugRequest(GeckoView view, GeckoView.PromptResult result) {
        result.cancel();
    }
}
