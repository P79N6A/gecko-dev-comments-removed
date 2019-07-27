



package org.mozilla.gecko.sync;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.content.SharedPreferences;





public class SharedPreferencesClientsDataDelegate implements ClientsDataDelegate {
  protected final SharedPreferences sharedPreferences;
  protected final Context context;

  public SharedPreferencesClientsDataDelegate(SharedPreferences sharedPreferences, Context context) {
    this.sharedPreferences = sharedPreferences;
    this.context = context;

    
    HardwareUtils.init(context);
  }

  @Override
  public synchronized String getAccountGUID() {
    String accountGUID = sharedPreferences.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
    if (accountGUID == null) {
      accountGUID = Utils.generateGuid();
      sharedPreferences.edit().putString(SyncConfiguration.PREF_ACCOUNT_GUID, accountGUID).commit();
    }
    return accountGUID;
  }

  




  @Override
  public synchronized void setClientName(String clientName, long now) {
    sharedPreferences
      .edit()
      .putString(SyncConfiguration.PREF_CLIENT_NAME, clientName)
      .putLong(SyncConfiguration.PREF_CLIENT_DATA_TIMESTAMP, now)
      .commit();
  }

  @Override
  public String getDefaultClientName() {
    String name = GlobalConstants.MOZ_APP_DISPLAYNAME; 
    
    if (name.contains("Aurora")) {
        name = "Aurora";
    } else if (name.contains("Beta")) {
        name = "Beta";
    } else if (name.contains("Nightly")) {
        name = "Nightly";
    }
    return context.getResources().getString(R.string.sync_default_client_name, name, android.os.Build.MODEL);
  }

  @Override
  public synchronized String getClientName() {
    String clientName = sharedPreferences.getString(SyncConfiguration.PREF_CLIENT_NAME, null);
    if (clientName == null) {
      clientName = getDefaultClientName();
      long now = System.currentTimeMillis();
      setClientName(clientName, now);
    }
    return clientName;
  }

  @Override
  public synchronized void setClientsCount(int clientsCount) {
    sharedPreferences.edit().putLong(SyncConfiguration.PREF_NUM_CLIENTS, clientsCount).commit();
  }

  @Override
  public boolean isLocalGUID(String guid) {
    return getAccountGUID().equals(guid);
  }

  @Override
  public synchronized int getClientsCount() {
    return (int) sharedPreferences.getLong(SyncConfiguration.PREF_NUM_CLIENTS, 0);
  }

  @Override
  public long getLastModifiedTimestamp() {
    return sharedPreferences.getLong(SyncConfiguration.PREF_CLIENT_DATA_TIMESTAMP, 0);
  }

  @Override
  public String getFormFactor() {
    if (HardwareUtils.isLargeTablet()) {
      return "largetablet";
    }

    if (HardwareUtils.isSmallTablet()) {
      return "smalltablet";
    }

    if (HardwareUtils.isTelevision()) {
      return "tv";
    }

    return "phone";
  }
}
