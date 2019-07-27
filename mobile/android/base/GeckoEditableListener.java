




package org.mozilla.gecko;





interface GeckoEditableListener {
    
    int NOTIFY_IME_OPEN_VKB = -2;
    int NOTIFY_IME_REPLY_EVENT = -1;
    int NOTIFY_IME_OF_FOCUS = 1;
    int NOTIFY_IME_OF_BLUR = 2;
    int NOTIFY_IME_TO_COMMIT_COMPOSITION = 8;
    int NOTIFY_IME_TO_CANCEL_COMPOSITION = 9;
    
    int IME_STATE_DISABLED = 0;
    int IME_STATE_ENABLED = 1;
    int IME_STATE_PASSWORD = 2;
    int IME_STATE_PLUGIN = 3;

    void notifyIME(int type);
    void notifyIMEContext(int state, String typeHint, String modeHint, String actionHint);
    void onSelectionChange(int start, int end);
    void onTextChange(CharSequence text, int start, int oldEnd, int newEnd);
}
