



package org.mozilla.gecko.db;

import java.util.ArrayList;

import android.os.Parcel;
import android.os.Parcelable;






public class RemoteClient implements Parcelable {
    public final String guid;
    public final String name;
    public final long lastModified;
    public final String deviceType;
    public final ArrayList<RemoteTab> tabs;

    public RemoteClient(String guid, String name, long lastModified, String deviceType) {
        this.guid = guid;
        this.name = name;
        this.lastModified = lastModified;
        this.deviceType = deviceType;
        this.tabs = new ArrayList<RemoteTab>();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int flags) {
        parcel.writeString(guid);
        parcel.writeString(name);
        parcel.writeLong(lastModified);
        parcel.writeString(deviceType);
        parcel.writeTypedList(tabs);
    }

    public static final Creator<RemoteClient> CREATOR = new Creator<RemoteClient>() {
        @Override
        public RemoteClient createFromParcel(final Parcel source) {
            final String guid = source.readString();
            final String name = source.readString();
            final long lastModified = source.readLong();
            final String deviceType = source.readString();

            final RemoteClient client = new RemoteClient(guid, name, lastModified, deviceType);
            source.readTypedList(client.tabs, RemoteTab.CREATOR);

            return client;
        }

        @Override
        public RemoteClient[] newArray(final int size) {
            return new RemoteClient[size];
        }
    };
}
