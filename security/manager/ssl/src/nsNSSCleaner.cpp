



#include "nsNSSCleaner.h"
#include "cert.h"

CERTVerifyLogContentsCleaner::CERTVerifyLogContentsCleaner(CERTVerifyLog *&cvl)
:m_cvl(cvl)
{
}

CERTVerifyLogContentsCleaner::~CERTVerifyLogContentsCleaner()
{
  if (!m_cvl)
    return;

  CERTVerifyLogNode *i_node;
  for (i_node = m_cvl->head; i_node; i_node = i_node->next)
  {
    if (i_node->cert)
      CERT_DestroyCertificate(i_node->cert);
  }
}

