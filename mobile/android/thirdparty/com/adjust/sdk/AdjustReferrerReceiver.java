package com.adjust.sdk;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import static com.adjust.sdk.Constants.ENCODING;
import static com.adjust.sdk.Constants.MALFORMED;
import static com.adjust.sdk.Constants.REFERRER;




public class AdjustReferrerReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        String rawReferrer = intent.getStringExtra(REFERRER);
        if (null == rawReferrer) {
            return;
        }

        String referrer;
        try {
            referrer = URLDecoder.decode(rawReferrer, ENCODING);
        } catch (UnsupportedEncodingException e) {
            referrer = MALFORMED;
        }

        AdjustInstance adjust = Adjust.getDefaultInstance();
        adjust.sendReferrer(referrer);
    }
}
