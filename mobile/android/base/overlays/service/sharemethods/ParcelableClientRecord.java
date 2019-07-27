



package org.mozilla.gecko.overlays.service.sharemethods;

import android.os.Parcel;
import android.os.Parcelable;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;







public class ParcelableClientRecord implements Parcelable {
    private static final String LOGTAG = "GeckoParcelableClientRecord";

    public final String name;
    public final String type;
    public final String guid;

    private ParcelableClientRecord(String aName, String aType, String aGUID) {
        name = aName;
        type = aType;
        guid = aGUID;
    }

    


    public static ParcelableClientRecord fromClientRecord(ClientRecord record) {
        return new ParcelableClientRecord(record.name, record.type, record.guid);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int flags) {
        parcel.writeString(name);
        parcel.writeString(type);
        parcel.writeString(guid);
    }

    public static final Creator<ParcelableClientRecord> CREATOR = new Creator<ParcelableClientRecord>() {
        @Override
        public ParcelableClientRecord createFromParcel(final Parcel source) {
            String name = source.readString();
            String type = source.readString();
            String guid = source.readString();

            return new ParcelableClientRecord(name, type, guid);
        }

        @Override
        public ParcelableClientRecord[] newArray(final int size) {
            return new ParcelableClientRecord[size];
        }
    };

    


    @Override
    public String toString() {
        return name;
    }
}
