















package com.googlecode.eyesfree.braille.selfbraille;

import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;





public class WriteData implements Parcelable {

    private static final String PROP_SELECTION_START = "selectionStart";
    private static final String PROP_SELECTION_END = "selectionEnd";

    private AccessibilityNodeInfo mAccessibilityNodeInfo;
    private CharSequence mText;
    private Bundle mProperties = Bundle.EMPTY;

    


    public static WriteData forView(View view) {
        AccessibilityNodeInfo node = AccessibilityNodeInfo.obtain(view);
        WriteData writeData = new WriteData();
        writeData.mAccessibilityNodeInfo = node;
        return writeData;
    }

    public static WriteData forInfo(AccessibilityNodeInfo info){
        WriteData writeData = new WriteData();
        writeData.mAccessibilityNodeInfo = info;
        return writeData;
    }


    public AccessibilityNodeInfo getAccessibilityNodeInfo() {
        return mAccessibilityNodeInfo;
    }

    





    public WriteData setText(CharSequence text) {
        mText = text;
        return this;
    }

    public CharSequence getText() {
        return mText;
    }

    




    public WriteData setSelectionStart(int v) {
        writableProperties().putInt(PROP_SELECTION_START, v);
        return this;
    }

    


    public int getSelectionStart() {
        return mProperties.getInt(PROP_SELECTION_START, -1);
    }

    








    public WriteData setSelectionEnd(int v) {
        writableProperties().putInt(PROP_SELECTION_END, v);
        return this;
    }

    


    public int getSelectionEnd() {
        return mProperties.getInt(PROP_SELECTION_END, -1);
    }

    private Bundle writableProperties() {
        if (mProperties == Bundle.EMPTY) {
            mProperties = new Bundle();
        }
        return mProperties;
    }

    




    public void validate() throws IllegalStateException {
        if (mAccessibilityNodeInfo == null) {
            throw new IllegalStateException(
                "Accessibility node info can't be null");
        }
        int selectionStart = getSelectionStart();
        int selectionEnd = getSelectionEnd();
        if (mText == null) {
            if (selectionStart > 0 || selectionEnd > 0) {
                throw new IllegalStateException(
                    "Selection can't be set without text");
            }
        } else {
            if (selectionStart < 0 && selectionEnd >= 0) {
                throw new IllegalStateException(
                    "Selection end without start");
            }
            int textLength = mText.length();
            if (selectionStart > textLength || selectionEnd > textLength) {
                throw new IllegalStateException("Selection out of bounds");
            }
        }
    }

    

    public static final Parcelable.Creator<WriteData> CREATOR =
        new Parcelable.Creator<WriteData>() {
            @Override
            public WriteData createFromParcel(Parcel in) {
                return new WriteData(in);
            }

            @Override
            public WriteData[] newArray(int size) {
                return new WriteData[size];
            }
        };

    @Override
    public int describeContents() {
        return 0;
    }

    




    @Override
    public void writeToParcel(Parcel out, int flags) {
        mAccessibilityNodeInfo.writeToParcel(out, flags);
        
        
        mAccessibilityNodeInfo = null;
        out.writeString(mText.toString());
        out.writeBundle(mProperties);
    }

    private WriteData() {
    }

    private WriteData(Parcel in) {
        mAccessibilityNodeInfo =
                AccessibilityNodeInfo.CREATOR.createFromParcel(in);
        mText = in.readString();
        mProperties = in.readBundle();
    }
}
