
























#ifndef MOZMEEGOAPPSERVICE_H
#define MOZMEEGOAPPSERVICE_H

#include <MApplicationService>







class MozMeegoAppService: public MApplicationService
{
  Q_OBJECT
public:
  MozMeegoAppService(): MApplicationService(QString()) {}
public Q_SLOTS:
  virtual QString registeredName() { return QString(); }
  virtual bool isRegistered() { return false; }
  virtual bool registerService() { return true; }
};
#endif 
