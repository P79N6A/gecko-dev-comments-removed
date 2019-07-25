



package org.mozilla.gecko.sync.delegates;

public interface ClientsDataDelegate {
  public String getAccountGUID();
  public String getClientName();
  public void setClientsCount(int clientsCount);
  public int getClientsCount();
  public boolean isLocalGUID(String guid);
}
