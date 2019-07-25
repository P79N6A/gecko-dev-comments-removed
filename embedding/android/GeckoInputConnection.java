




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;

import android.util.*;

public class GeckoInputConnection
    extends BaseInputConnection
{
    public GeckoInputConnection (View targetView) {
        super(targetView, true);
        mQueryResult = new SynchronousQueue<String>();
        mExtractedText.partialStartOffset = -1;
        mExtractedText.partialEndOffset = -1;
    }

    @Override
    public Editable getEditable() {
        Log.i("GeckoAppJava", "getEditable");
        return null;
    }

    @Override
    public boolean beginBatchEdit() {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, null));
        return true;
    }
    @Override
    public boolean commitCompletion(CompletionInfo text) {
        Log.i("GeckoAppJava", "Stub: commitCompletion");
        return true;
    }
    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, text.toString()));
        endBatchEdit();
        return true;
    }
    @Override
    public boolean deleteSurroundingText(int leftLength, int rightLength) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(leftLength, rightLength));
        updateExtractedText();
        return true;
    }
    @Override
    public boolean endBatchEdit() {
        updateExtractedText();
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, null));
        return true;
    }
    @Override
    public boolean finishComposingText() {
        endBatchEdit();
        return true;
    }
    @Override
    public int getCursorCapsMode(int reqModes) {
        return 0;
    }
    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest req, int flags) {
        mExtractToken = req.token;
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, 0));
        try {
            mExtractedText.text = mQueryResult.take();
            mExtractedText.selectionStart = mSelectionStart;
            mExtractedText.selectionEnd = mSelectionEnd;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getExtractedText: Interrupted!");
        }
        return mExtractedText;
    }
    @Override
    public CharSequence getTextAfterCursor(int length, int flags) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, length));
        try {
            String result = mQueryResult.take();
            return result;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getTextAfterCursor: Interrupted!");
        }
        return null;
    }
    @Override
    public CharSequence getTextBeforeCursor(int length, int flags) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, length));
        try {
            String result = mQueryResult.take();
            return result;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getTextBeforeCursor: Interrupted!");
        }
        return null;
    }
    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        beginBatchEdit();
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, text.toString()));
        return true;
    }
    @Override
    public boolean setSelection(int start, int end) {
        Log.i("GeckoAppJava", "Stub: setSelection " + start + " " + end);
        return true;
    }

    private void updateExtractedText() {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, 0));
        try {
            mExtractedText.text = mQueryResult.take();
            mExtractedText.selectionStart = mSelectionStart;
            mExtractedText.selectionEnd = mSelectionEnd;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getExtractedText: Interrupted!");
        }

        InputMethodManager imm = (InputMethodManager)
            GeckoApp.surfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.updateExtractedText(GeckoApp.surfaceView, mExtractToken, mExtractedText);
    }

    int mExtractToken;
    final ExtractedText mExtractedText = new ExtractedText();

    int mSelectionStart, mSelectionEnd;
    SynchronousQueue<String> mQueryResult;
}
