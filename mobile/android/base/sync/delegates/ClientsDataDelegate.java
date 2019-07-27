



package org.mozilla.gecko.sync.delegates;

public interface ClientsDataDelegate {
  public String getAccountGUID();
  public String getDefaultClientName();
  public void setClientName(String clientName, long now);
  public String getClientName();
  public void setClientsCount(int clientsCount);
  public int getClientsCount();
  public boolean isLocalGUID(String guid);
  public String getFormFactor();

  









  public long getLastModifiedTimestamp();
}
