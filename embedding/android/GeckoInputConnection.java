




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;

import android.os.*;
import android.app.*;
import android.text.*;
import android.text.style.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.R;

import android.util.*;

public class GeckoInputConnection
    extends BaseInputConnection
    implements TextWatcher
{
    public GeckoInputConnection (View targetView) {
        super(targetView, true);
        mQueryResult = new SynchronousQueue<String>();
    }

    @Override
    public boolean beginBatchEdit() {
        

        return true;
    }

    @Override
    public boolean commitCompletion(CompletionInfo text) {
        

        return commitText(text.getText(), 1);
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        

        setComposingText(text, newCursorPosition);
        finishComposingText();

        return true;
    }

    @Override
    public boolean deleteSurroundingText(int leftLength, int rightLength) {
        

        



        if (mComposing) {
            
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(0, 0, 0, 0, 0, 0, null));
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_COMPOSITION_END, 0, 0));
        }

        
        int delStart, delLen;
        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_GET_SELECTION, 0, 0));
        try {
            mQueryResult.take();
        } catch (InterruptedException e) {
            Log.e("GeckoAppJava", "IME: deleteSurroundingText interrupted", e);
            return false;
        }
        delStart = mSelectionStart > leftLength ?
                    mSelectionStart - leftLength : 0;
        delLen = mSelectionStart + rightLength - delStart;
        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_SET_SELECTION, delStart, delLen));

        
        if (mComposing) {
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_COMPOSITION_BEGIN, 0, 0));
            if (mComposingText.length() > 0) {
                
                GeckoAppShell.sendEventToGecko(
                    new GeckoEvent(0, mComposingText.length(),
                                   GeckoEvent.IME_RANGE_RAWINPUT,
                                   GeckoEvent.IME_RANGE_UNDERLINE, 0, 0,
                                   mComposingText.toString()));
            }
        } else {
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_DELETE_TEXT, 0, 0));
        }
        return true;
    }

    @Override
    public boolean endBatchEdit() {
        

        return true;
    }

    @Override
    public boolean finishComposingText() {
        

        if (mComposing) {
            
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(0, mComposingText.length(),
                               GeckoEvent.IME_RANGE_RAWINPUT, 0, 0, 0,
                               mComposingText));
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_COMPOSITION_END, 0, 0));
            mComposing = false;
            mComposingText = "";

            
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_SET_SELECTION,
                               mCompositionStart + mCompositionSelStart, 0));
        }
        return true;
    }

    @Override
    public int getCursorCapsMode(int reqModes) {
        

        return 0;
    }

    @Override
    public Editable getEditable() {
        Log.w("GeckoAppJava", "IME: getEditable called from " +
            Thread.currentThread().getStackTrace()[0].toString());

        return null;
    }

    @Override
    public boolean performContextMenuAction(int id) {
        

        
        
        String text;
        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_GET_TEXT, 0, Integer.MAX_VALUE));
        try {
            text = mQueryResult.take();
        } catch (InterruptedException e) {
            Log.e("GeckoAppJava", "IME: performContextMenuAction interrupted", e);
            return false;
        }

        switch (id) {
            case R.id.selectAll:
                setSelection(0, text.length());
                break;
            case R.id.cut:
                
                GeckoAppShell.setClipboardText(text);
                
                if (mSelectionLength <= 0)
                    GeckoAppShell.sendEventToGecko(
                        new GeckoEvent(GeckoEvent.IME_SET_SELECTION, 0, text.length()));
                GeckoAppShell.sendEventToGecko(
                    new GeckoEvent(GeckoEvent.IME_DELETE_TEXT, 0, 0));
                break;
            case R.id.paste:
                commitText(GeckoAppShell.getClipboardText(), 1);
                break;
            case R.id.copy:
                
                
                if (mSelectionLength > 0) {
                    GeckoAppShell.sendEventToGecko(
                        new GeckoEvent(GeckoEvent.IME_GET_SELECTION, 0, 0));
                    try {
                        text = mQueryResult.take();
                    } catch (InterruptedException e) {
                        Log.e("GeckoAppJava", "IME: performContextMenuAction interrupted", e);
                        return false;
                    }
                }
                GeckoAppShell.setClipboardText(text);
                break;
        }
        return true;
    }

    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest req, int flags) {
        if (req == null)
            return null;

        

        ExtractedText extract = new ExtractedText();
        extract.flags = 0;
        extract.partialStartOffset = -1;
        extract.partialEndOffset = -1;

        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_GET_SELECTION, 0, 0));
        try {
            mQueryResult.take();
        } catch (InterruptedException e) {
            Log.e("GeckoAppJava", "IME: getExtractedText interrupted", e);
            return null;
        }
        extract.selectionStart = mSelectionStart;
        extract.selectionEnd = mSelectionStart + mSelectionLength;

        
        
        
        
        try {
            Thread.sleep(20);
        } catch (InterruptedException e) {}

        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_GET_TEXT, 0, Integer.MAX_VALUE));
        try {
            extract.startOffset = 0;
            extract.text = mQueryResult.take();

            
            if (mComposing && extract.selectionEnd > extract.text.length())
                extract.text = extract.text.subSequence(0, mCompositionStart) + mComposingText;

            
            extract.selectionStart = Math.min(extract.selectionStart, extract.text.length());
            extract.selectionEnd = Math.min(extract.selectionEnd, extract.text.length());

            if ((flags & GET_EXTRACTED_TEXT_MONITOR) != 0)
                mUpdateRequest = req;
            return extract;

        } catch (InterruptedException e) {
            Log.e("GeckoAppJava", "IME: getExtractedText interrupted", e);
            return null;
        }
    }

    @Override
    public CharSequence getTextAfterCursor(int length, int flags) {
        

        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_GET_SELECTION, 0, 0));
        try {
            mQueryResult.take();
        } catch (InterruptedException e) {
            Log.e("GeckoAppJava", "IME: getTextBefore/AfterCursor interrupted", e);
            return null;
        }

        

        int textStart = mSelectionStart;
        int textLength = length;

        if (length < 0) {
          textStart += length;
          textLength = -length;
          if (textStart < 0) {
            textStart = 0;
            textLength = mSelectionStart;
          }
        }

        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_GET_TEXT, textStart, textLength));
        try {
            return mQueryResult.take();
        } catch (InterruptedException e) {
            Log.e("GeckoAppJava", "IME: getTextBefore/AfterCursor: Interrupted!", e);
            return null;
        }
    }

    @Override
    public CharSequence getTextBeforeCursor(int length, int flags) {
        

        return getTextAfterCursor(-length, flags);
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        

        
        mComposingText = text != null ? text.toString() : "";

        if (!mComposing) {
            
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_GET_SELECTION, 0, 0));
            try {
                mQueryResult.take();
            } catch (InterruptedException e) {
                Log.e("GeckoAppJava", "IME: setComposingText interrupted", e);
                return false;
            }
            
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_COMPOSITION_BEGIN, 0, 0));
            mComposing = true;
            mCompositionStart = mSelectionLength >= 0 ?
                mSelectionStart : mSelectionStart + mSelectionLength;
        }

        
        
        mCompositionSelStart = newCursorPosition > 0 ? mComposingText.length() : 0;
        mCompositionSelLen = 0;

        
        if (text != null && text instanceof Spanned) {
            Spanned span = (Spanned) text;
            int spanStart = 0, spanEnd = 0;
            boolean pastSelStart = false, pastSelEnd = false;

            do {
                int rangeType = GeckoEvent.IME_RANGE_CONVERTEDTEXT;
                int rangeStyles = 0, rangeForeColor = 0, rangeBackColor = 0;

                
                spanEnd = span.nextSpanTransition(spanStart + 1, text.length(),
                    CharacterStyle.class);

                
                if (mCompositionSelLen >= 0) {
                    if (!pastSelStart && spanEnd >= mCompositionSelStart) {
                        spanEnd = mCompositionSelStart;
                        pastSelStart = true;
                    } else if (!pastSelEnd && spanEnd >=
                            mCompositionSelStart + mCompositionSelLen) {
                        spanEnd = mCompositionSelStart + mCompositionSelLen;
                        pastSelEnd = true;
                        rangeType = GeckoEvent.IME_RANGE_SELECTEDRAWTEXT;
                    }
                } else {
                    if (!pastSelEnd && spanEnd >=
                            mCompositionSelStart + mCompositionSelLen) {
                        spanEnd = mCompositionSelStart + mCompositionSelLen;
                        pastSelEnd = true;
                    } else if (!pastSelStart &&
                            spanEnd >= mCompositionSelStart) {
                        spanEnd = mCompositionSelStart;
                        pastSelStart = true;
                        rangeType = GeckoEvent.IME_RANGE_SELECTEDRAWTEXT;
                    }
                }
                
                if (spanEnd <= spanStart)
                    continue;

                
                CharacterStyle styles[] = span.getSpans(
                    spanStart, spanEnd, CharacterStyle.class);

                for (CharacterStyle style : styles) {
                    if (style instanceof UnderlineSpan) {
                        
                        rangeStyles |= GeckoEvent.IME_RANGE_UNDERLINE;

                    } else if (style instanceof ForegroundColorSpan) {
                        
                        rangeStyles |= GeckoEvent.IME_RANGE_FORECOLOR;
                        rangeForeColor =
                            ((ForegroundColorSpan)style).getForegroundColor();

                    } else if (style instanceof BackgroundColorSpan) {
                        
                        rangeStyles |= GeckoEvent.IME_RANGE_BACKCOLOR;
                        rangeBackColor =
                            ((BackgroundColorSpan)style).getBackgroundColor();
                    }
                }

                
                
                GeckoAppShell.sendEventToGecko(
                    new GeckoEvent(spanStart, spanEnd - spanStart,
                                   rangeType, rangeStyles,
                                   rangeForeColor, rangeBackColor));

                spanStart = spanEnd;
            } while (spanStart < text.length());
        } else {
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(0, text == null ? 0 : text.length(),
                               GeckoEvent.IME_RANGE_RAWINPUT,
                               GeckoEvent.IME_RANGE_UNDERLINE, 0, 0));
        }

        
        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(mCompositionSelStart + mCompositionSelLen, 0,
                           GeckoEvent.IME_RANGE_CARETPOSITION, 0, 0, 0,
                           mComposingText));
        return true;
    }

    @Override
    public boolean setSelection(int start, int end) {
        

        if (mComposing) {
            
            start -= mCompositionStart;
            end -= mCompositionStart;

            if (start < 0)
                start = 0;
            else if (start > mComposingText.length())
                start = mComposingText.length();

            if (end < 0)
                end = 0;
            else if (end > mComposingText.length())
                end = mComposingText.length();

            mCompositionSelStart = start;
            mCompositionSelLen = end - start;
        } else {
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_SET_SELECTION,
                               start, end - start));
        }
        return true;
    }

    public boolean onKeyDel() {
        
        
        

        if (!mComposing)
            return false;

        if (mComposingText.length() > 0) {
            mComposingText = mComposingText.substring(0,
                mComposingText.length() - 1);
            if (mComposingText.length() > 0)
                return false;
        }

        commitText(null, 1);
        return true;
    }

    public void notifyTextChange(InputMethodManager imm, String text,
                                 int start, int oldEnd, int newEnd) {
        

        if (!text.contentEquals(GeckoApp.surfaceView.mEditable))
            GeckoApp.surfaceView.setEditable(text);

        if (mUpdateRequest == null)
            return;

        mUpdateExtract.flags = 0;

        
        
        mUpdateExtract.partialStartOffset = 0;
        mUpdateExtract.partialEndOffset = oldEnd;

        
        mUpdateExtract.selectionStart = newEnd;
        mUpdateExtract.selectionEnd = newEnd;

        mUpdateExtract.text = text.substring(0, newEnd);
        mUpdateExtract.startOffset = 0;

        imm.updateExtractedText(GeckoApp.surfaceView,
            mUpdateRequest.token, mUpdateExtract);
    }

    public void notifySelectionChange(InputMethodManager imm,
                                      int start, int end) {
        

        if (mComposing)
            imm.updateSelection(GeckoApp.surfaceView,
                mCompositionStart + mCompositionSelStart,
                mCompositionStart + mCompositionSelStart + mCompositionSelLen,
                mCompositionStart,
                mCompositionStart + mComposingText.length());
        else
            imm.updateSelection(GeckoApp.surfaceView, start, end, -1, -1);

        int maxLen = GeckoApp.surfaceView.mEditable.length();
        Selection.setSelection(GeckoApp.surfaceView.mEditable, 
                               Math.min(start, maxLen),
                               Math.min(end, maxLen));
    }

    public void reset() {
        mComposing = false;
        mComposingText = "";
        mUpdateRequest = null;
    }

    
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
        GeckoAppShell.sendEventToGecko(
            new GeckoEvent(GeckoEvent.IME_SET_SELECTION, start, before));

        if (count == 0) {
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_DELETE_TEXT, 0, 0));
        } else {
            
            finishComposingText();
            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_COMPOSITION_BEGIN, 0, 0));

            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(0, count,
                               GeckoEvent.IME_RANGE_RAWINPUT, 0, 0, 0,
                               s.subSequence(start, start + count).toString()));

            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_COMPOSITION_END, 0, 0));

            GeckoAppShell.sendEventToGecko(
                new GeckoEvent(GeckoEvent.IME_SET_SELECTION, start + count, 0));
        }

        
        GeckoAppShell.geckoEventSync();
    }

    public void afterTextChanged(Editable s)
    {
    }

    public void beforeTextChanged(CharSequence s, int start, int count, int after)
    {
    }

    
    boolean mComposing;
    
    String mComposingText = "";
    
    int mCompositionStart;
    

    
    int mCompositionSelStart;
    
    int mCompositionSelLen;

    ExtractedTextRequest mUpdateRequest;
    final ExtractedText mUpdateExtract = new ExtractedText();

    int mSelectionStart, mSelectionLength;
    SynchronousQueue<String> mQueryResult;
}

