







































#include "nsAutoPtr.h"
#include "nsWifiMonitor.h"
#include "nsWifiAccessPoint.h"

#include "nsIProxyObjectManager.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"

#include "plstr.h"
#include <glib.h>

#define DLADM_STRSIZE 256
#define DLADM_SECTIONS 3

struct val_strength_t {
  const char *strength_name;
  int         signal_value;
};

static val_strength_t strength_vals[] = {
  { "very weak", -112 },
  { "weak",      -88 },
  { "good",      -68 },
  { "very good", -40 },
  { "excellent", -16 }
};

static nsWifiAccessPoint *
do_parse_str(char *bssid_str, char *essid_str, char *strength)
{
  unsigned char mac_as_int[6] = { 0 };
  sscanf(bssid_str, "%x:%x:%x:%x:%x:%x", &mac_as_int[0], &mac_as_int[1],
         &mac_as_int[2], &mac_as_int[3], &mac_as_int[4], &mac_as_int[5]);

  int signal = 0;
  PRUint32 strength_vals_count = sizeof(strength_vals) / sizeof (val_strength_t);
  for (PRUint32 i = 0; i < strength_vals_count; i++) {
    if (!strncasecmp(strength, strength_vals[i].strength_name, DLADM_STRSIZE)) {
      signal = strength_vals[i].signal_value;
      break;
    }
  }

  nsWifiAccessPoint *ap = new nsWifiAccessPoint();
  if (ap) {
    ap->setMac(mac_as_int);
    ap->setSignal(signal);
    ap->setSSID(essid_str, PL_strnlen(essid_str, DLADM_STRSIZE));
  }
  return ap;
}

static void
do_dladm(nsCOMArray<nsWifiAccessPoint> &accessPoints)
{
  GError *err = NULL;
  char *sout = NULL;
  char *serr = NULL;
  int exit_status = 0;
  char * dladm_args[] = { "/usr/bin/pfexec", "/usr/sbin/dladm",
                          "scan-wifi", "-p", "-o", "BSSID,ESSID,STRENGTH" };

  gboolean rv = g_spawn_sync("/", dladm_args, NULL, (GSpawnFlags)0, NULL, NULL,
                             &sout, &serr, &exit_status, &err);
  if (rv && !exit_status) {
    char wlan[DLADM_SECTIONS][DLADM_STRSIZE+1];
    PRUint32 section = 0;
    PRUint32 sout_scan = 0;
    PRUint32 wlan_put = 0;
    PRBool escape = PR_FALSE;
    nsWifiAccessPoint* ap;
    char sout_char;
    do {
      sout_char = sout[sout_scan++];
      if (escape) {
        escape = PR_FALSE;
        if (sout_char != '\0') {
          wlan[section][wlan_put++] = sout_char;
          continue;
        }
      }

      if (sout_char =='\\') {
        escape = PR_TRUE;
        continue;
      }

      if (sout_char == ':') {
        wlan[section][wlan_put] = '\0';
        section++;
        wlan_put = 0;
        continue;
      }

      if ((sout_char == '\0') || (sout_char == '\n')) {
        wlan[section][wlan_put] = '\0';
        if (section == DLADM_SECTIONS - 1) {
          ap = do_parse_str(wlan[0], wlan[1], wlan[2]);
          if (ap) {
            accessPoints.AppendObject(ap);
          }
        }
        section = 0;
        wlan_put = 0;
        continue;
      }

      wlan[section][wlan_put++] = sout_char;

    } while ((wlan_put <= DLADM_STRSIZE) && (section < DLADM_SECTIONS) &&
             (sout_char != '\0'));
  }

  g_free(sout);
  g_free(serr);
}

nsresult
nsWifiMonitor::DoScan()
{
  

  nsCOMArray<nsWifiAccessPoint> lastAccessPoints;
  nsCOMArray<nsWifiAccessPoint> accessPoints;

  while (mKeepGoing) {

    accessPoints.Clear();
    do_dladm(accessPoints);

    PRBool accessPointsChanged = !AccessPointsEqual(accessPoints, lastAccessPoints);
    nsCOMArray<nsIWifiListener> currentListeners;

    {
      nsAutoMonitor mon(mMonitor);

      for (PRUint32 i = 0; i < mListeners.Length(); i++) {
        if (!mListeners[i].mHasSentData || accessPointsChanged) {
          mListeners[i].mHasSentData = PR_TRUE;
          currentListeners.AppendObject(mListeners[i].mListener);
        }
      }
    }

    ReplaceArray(lastAccessPoints, accessPoints);

    if (currentListeners.Count() > 0)
    {
      PRUint32 resultCount = lastAccessPoints.Count();
      nsIWifiAccessPoint** result = static_cast<nsIWifiAccessPoint**> (nsMemory::Alloc(sizeof(nsIWifiAccessPoint*) * resultCount));
      if (!result) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      for (PRUint32 i = 0; i < resultCount; i++)
        result[i] = lastAccessPoints[i];

      for (PRInt32 i = 0; i < currentListeners.Count(); i++) {

        LOG(("About to send data to the wifi listeners\n"));

        nsCOMPtr<nsIWifiListener> proxy;
        nsCOMPtr<nsIProxyObjectManager> proxyObjMgr = do_GetService("@mozilla.org/xpcomproxy;1");
        proxyObjMgr->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                       NS_GET_IID(nsIWifiListener),
                                       currentListeners[i],
                                       NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(proxy));
        if (!proxy) {
          LOG(("There is no proxy available.  this should never happen\n"));
        }
        else
        {
          nsresult rv = proxy->OnChange(result, resultCount);
          LOG( ("... sent %d\n", rv));
        }
      }

      nsMemory::Free(result);
    }

    LOG(("waiting on monitor\n"));

    nsAutoMonitor mon(mMonitor);
    mon.Wait(PR_SecondsToInterval(60));
  }

  return NS_OK;
}
