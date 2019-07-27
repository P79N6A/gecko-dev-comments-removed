




package org.mozilla.gecko.home;

import java.util.HashSet;
import java.util.Set;

import org.mozilla.gecko.util.PrefUtils;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;















public class RemoteTabsExpandableListState {
    private static final String PREF_COLLAPSED_CLIENT_GUIDS = "remote_tabs_collapsed_client_guids";
    private static final String PREF_HIDDEN_CLIENT_GUIDS = "remote_tabs_hidden_client_guids";

    protected final SharedPreferences sharedPrefs;

    
    
    
    protected final Set<String> collapsedClients;

    
    
    
    protected final Set<String> hiddenClients;

    public RemoteTabsExpandableListState(SharedPreferences sharedPrefs) {
        if (null == sharedPrefs) {
            throw new IllegalArgumentException("sharedPrefs must not be null");
        }
        this.sharedPrefs = sharedPrefs;

        this.collapsedClients = getStringSet(PREF_COLLAPSED_CLIENT_GUIDS);
        this.hiddenClients = getStringSet(PREF_HIDDEN_CLIENT_GUIDS);
    }

    







    protected Set<String> getStringSet(String pref) {
        final Set<String> loaded = PrefUtils.getStringSet(sharedPrefs, pref, null);
        if (loaded != null) {
            return new HashSet<String>(loaded);
        } else {
            return new HashSet<String>();
        }
    }

    












    protected boolean updateClientMembership(String pref, Set<String> clients, String clientGuid, boolean isMember) {
        final boolean modified;
        if (isMember) {
            modified = clients.add(clientGuid);
        } else {
            modified = clients.remove(clientGuid);
        }

        if (modified) {
            
            
            
            final Editor editor = sharedPrefs.edit();
            PrefUtils.putStringSet(editor, pref, clients);
            editor.apply();
        }

        return modified;
    }

    








    protected synchronized boolean setClientCollapsed(String clientGuid, boolean collapsed) {
        return updateClientMembership(PREF_COLLAPSED_CLIENT_GUIDS, collapsedClients, clientGuid, collapsed);
    }

    public synchronized boolean isClientCollapsed(String clientGuid) {
        return collapsedClients.contains(clientGuid);
    }

    








    protected synchronized boolean setClientHidden(String clientGuid, boolean hidden) {
        return updateClientMembership(PREF_HIDDEN_CLIENT_GUIDS, hiddenClients, clientGuid, hidden);
    }

    public synchronized boolean isClientHidden(String clientGuid) {
        return hiddenClients.contains(clientGuid);
    }
}
