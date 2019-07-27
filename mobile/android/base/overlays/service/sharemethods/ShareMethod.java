



package org.mozilla.gecko.overlays.service.sharemethods;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;
import org.mozilla.gecko.overlays.service.ShareData;




public abstract class ShareMethod {
    protected final Context context;

    public ShareMethod(Context aContext) {
        context = aContext;
    }

    











    public abstract Result handle(ShareData shareData);

    public abstract String getSuccessMesssage();
    public abstract String getFailureMessage();

    


    public static enum Result {
        
        SUCCESS,

        
        TRANSIENT_FAILURE,

        
        
        
        
        PERMANENT_FAILURE
    }

    


    public static enum Type implements Parcelable {
        ADD_BOOKMARK,
        ADD_TO_READING_LIST,
        SEND_TAB;

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(final Parcel dest, final int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<Type> CREATOR = new Creator<Type>() {
            @Override
            public Type createFromParcel(final Parcel source) {
                return Type.values()[source.readInt()];
            }

            @Override
            public Type[] newArray(final int size) {
                return new Type[size];
            }
        };
    }
}
