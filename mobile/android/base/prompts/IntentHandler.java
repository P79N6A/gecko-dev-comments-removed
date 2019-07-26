



package org.mozilla.gecko.prompts;

import android.content.Intent;

public interface IntentHandler {
    public void onIntentSelected(Intent intent, int position);
    public void onCancelled();
}
