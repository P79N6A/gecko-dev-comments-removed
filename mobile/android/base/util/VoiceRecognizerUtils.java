




package org.mozilla.gecko.util;

import android.content.Context;
import android.content.Intent;
import android.speech.RecognizerIntent;

public class VoiceRecognizerUtils {
    public static boolean supportsVoiceRecognizer(Context context, String prompt) {
        final Intent intent = createVoiceRecognizerIntent(prompt);
        return intent.resolveActivity(context.getPackageManager()) != null;
    }

    public static Intent createVoiceRecognizerIntent(String prompt) {
        final Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH);
        intent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 1);
        intent.putExtra(RecognizerIntent.EXTRA_PROMPT, prompt);
        return intent;
    }
}
