

























#include "graphite2/Font.h"
#include "Face.h"
#include "FeatureMap.h"
#include "FeatureVal.h"
#include "NameTable.h"

using namespace graphite2;

extern "C" {


gr_uint16 gr_fref_feature_value(const gr_feature_ref* pfeatureref, const gr_feature_val* feats)    
{
    if (!pfeatureref)
    return 0;
    if (!feats)
    return 0;
    
    return pfeatureref->getFeatureVal(*feats);
}


int gr_fref_set_feature_value(const gr_feature_ref* pfeatureref, gr_uint16 val, gr_feature_val* pDest)
{
    if (!pfeatureref)
    return false;
    if (!pDest)
    return false;
    
    return pfeatureref->applyValToFeature(val, *pDest);
}


gr_uint32 gr_fref_id(const gr_feature_ref* pfeatureref)    
{
  if (!pfeatureref)
    return 0;
  
  return pfeatureref->getId();
}


gr_uint16 gr_fref_n_values(const gr_feature_ref* pfeatureref)
{
    if(!pfeatureref)
        return 0;
    return pfeatureref->getNumSettings();
}


gr_int16 gr_fref_value(const gr_feature_ref* pfeatureref, gr_uint16 settingno)
{
    if(!pfeatureref || (settingno >= pfeatureref->getNumSettings()))
    {
        return 0;
    }
    return pfeatureref->getSettingValue(settingno);
}


void* gr_fref_label(const gr_feature_ref* pfeatureref, gr_uint16 *langId, gr_encform utf, gr_uint32 *length)
{
    if(!pfeatureref || !pfeatureref->getFace())
    {
        langId = 0;
        length = 0;
        return NULL;
    }
    uint16 label = pfeatureref->getNameId();
    NameTable * names = pfeatureref->getFace()->nameTable();
    if (!names)
    {
        langId = 0;
        length = 0;
        return NULL;
    }
    return names->getName(*langId, label, utf, *length);
}


void* gr_fref_value_label(const gr_feature_ref*pfeatureref, gr_uint16 setting,
    gr_uint16 *langId, gr_encform utf, gr_uint32 *length)
{
    if(!pfeatureref || (setting >= pfeatureref->getNumSettings()) || !pfeatureref->getFace())
    {
        langId = 0;
        length = 0;
        return NULL;
    }
    uint16 label = pfeatureref->getSettingName(setting);
    NameTable * names = pfeatureref->getFace()->nameTable();
    if (!names)
    {
        langId = 0;
        length = 0;
        return NULL;
    }
    return names->getName(*langId, label, utf, *length);
}


void gr_label_destroy(void * label)
{
	free(label);
}

gr_feature_val* gr_featureval_clone(const gr_feature_val* pfeatures)
{                      
    return static_cast<gr_feature_val*>(pfeatures ? new Features(*pfeatures) : new Features);
}
  
void gr_featureval_destroy(gr_feature_val *p)
{
    delete p;
}


} 
