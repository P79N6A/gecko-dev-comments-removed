






































#include <libxml/parser.h>
#include <libxml/tree.h>
 
#include "capability_set.h"
#include "CCProvider.h"






static cc_boolean  capability_set[MAX_CALL_STATES][CCAPI_CALL_CAP_MAX];
static cc_boolean  capability_idleset[CCAPI_CALL_CAP_MAX];








#define FCP_FEATURE_MAX      9

char                                    g_fp_version_stamp[MAX_FP_VERSION_STAMP_LEN];
static cc_feature_control_policy_info_t cc_feat_control_policy[FCP_FEATURE_MAX];
static ccapi_call_capability_e          cc_fcp_id_to_capability_map[FCP_FEATURE_MAX+1];    


static const unsigned int               CALL_FORWARD_ALL_FCP_INDEX  = 1;
static const unsigned int               CONFERENCE_LIST_FCP_INDEX   = 4;
static const unsigned int               SPEED_DIAL_FCP_INDEX        = 5;
static const unsigned int               CALL_BACK_FCP_INDEX         = 6;
static const unsigned int               REDIAL_FCP_INDEX            = 7;
  
static int                              fcp_index = -1;








static void capset_set_fcp_forwardall (cc_boolean state)
{
   CONFIG_DEBUG(DEB_F_PREFIX"FCP Setting CALLFWD Capability to [%d]\n", DEB_F_PREFIX_ARGS(JNI, "capset_set_fcp_forwardall"), (unsigned int)state);

   capability_idleset[CCAPI_CALL_CAP_CALLFWD]       = state;
   capability_set[OFFHOOK][CCAPI_CALL_CAP_CALLFWD]  = state;
}





static void capset_set_fcp_redial (cc_boolean state)
{
   CONFIG_DEBUG(DEB_F_PREFIX"FCP Setting REDIAL capability to [%d]\n", DEB_F_PREFIX_ARGS(JNI, "capset_set_fcp_redial"), (unsigned int)state);

   capability_idleset[CCAPI_CALL_CAP_REDIAL]        = state;
   capability_set[OFFHOOK][CCAPI_CALL_CAP_REDIAL]   = state;
   capability_set[ONHOOK][CCAPI_CALL_CAP_REDIAL]    = state;
}









static void fcp_set_index (unsigned int fcpCapabilityId, cc_boolean state)
{
   ccapi_call_capability_e capabilityId = CCAPI_CALL_CAP_MAX;

   
   if ((fcpCapabilityId <= 0) || (fcpCapabilityId > FCP_FEATURE_MAX))
   {
        CONFIG_ERROR(CFG_F_PREFIX "Unable to set capability of unknown feature [%d] in FCP \n", "fcp_set_index", fcpCapabilityId);                    
        return;   
   }

   
   capabilityId = cc_fcp_id_to_capability_map[fcpCapabilityId];
   
       
   
   switch (capabilityId)
   {
       case CCAPI_CALL_CAP_CALLFWD  :  capset_set_fcp_forwardall (state);      break;
       case CCAPI_CALL_CAP_REDIAL   :  capset_set_fcp_redial (state);          break;
       default :
       {
           CONFIG_ERROR(CFG_F_PREFIX "Unable to update settings for capability [%d]\n", "fcp_set_index", (int)capabilityId);                    
           break;
       }
    }
}





static void capset_init () 
{
   
   memset(capability_idleset, 0, sizeof(capability_idleset));
   memset(capability_set, 0, sizeof(capability_set));

   
   
   
   
   CONFIG_DEBUG(DEB_F_PREFIX"FCP Initializing Capabilities to default\n", DEB_F_PREFIX_ARGS(JNI, "capset_init"));
      
   
   
   
   
   
   
   
   capability_idleset[CCAPI_CALL_CAP_NEWCALL]                    = TRUE;

   
   
   capability_set[OFFHOOK][CCAPI_CALL_CAP_ENDCALL]               = TRUE;

   
   capability_set[ONHOOK][CCAPI_CALL_CAP_NEWCALL] = TRUE;

   
   capability_set[RINGOUT][CCAPI_CALL_CAP_ENDCALL] = TRUE;

   
   capability_set[RINGIN][CCAPI_CALL_CAP_ANSWER] = TRUE;

   
   capability_set[PROCEED][CCAPI_CALL_CAP_ENDCALL] = TRUE;

   
   capability_set[CONNECTED][CCAPI_CALL_CAP_ENDCALL] = TRUE;
   capability_set[CONNECTED][CCAPI_CALL_CAP_HOLD] = TRUE;
   capability_set[CONNECTED][CCAPI_CALL_CAP_TRANSFER] = TRUE;
   capability_set[CONNECTED][CCAPI_CALL_CAP_CONFERENCE] = TRUE;
   capability_set[CONNECTED][CCAPI_CALL_CAP_SELECT] = TRUE;

   
   capability_set[HOLD][CCAPI_CALL_CAP_RESUME] = TRUE;
   capability_set[REMHOLD][CCAPI_CALL_CAP_RESUME] = TRUE;

   
   capability_set[BUSY][CCAPI_CALL_CAP_ENDCALL] = TRUE;

   
   capability_set[REORDER][CCAPI_CALL_CAP_ENDCALL] = TRUE;

   
   capability_set[DIALING][CCAPI_CALL_CAP_ENDCALL] = TRUE;
   capability_set[DIALING][CCAPI_CALL_CAP_DIAL] = TRUE;
   capability_set[DIALING][CCAPI_CALL_CAP_SENDDIGIT] = TRUE;
   capability_set[DIALING][CCAPI_CALL_CAP_BACKSPACE] = TRUE;

   
   capability_set[HOLDREVERT][CCAPI_CALL_CAP_ANSWER] = TRUE;

   
   capability_set[PRESERVATION][CCAPI_CALL_CAP_ENDCALL] = TRUE;

   
   capability_set[WAITINGFORDIGITS][CCAPI_CALL_CAP_SENDDIGIT] = TRUE;
   capability_set[WAITINGFORDIGITS][CCAPI_CALL_CAP_BACKSPACE] = TRUE;
}






void capset_get_idleset ( cc_cucm_mode_t mode, cc_boolean features[])
{
  static const char fname[] = "capset_get_idleset";
  int i;

  CCAPP_DEBUG(DEB_F_PREFIX"updating idleset",
            DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

     for (i=0;i < CCAPI_CALL_CAP_MAX; i++) {
  CCAPP_DEBUG(DEB_F_PREFIX"updating line features %d=%d",
            DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), i, capability_idleset[i]);
       features[i] = capability_idleset[i];
     }

}





void capset_get_allowed_features ( cc_cucm_mode_t mode, cc_call_state_t state, cc_boolean features[]) 
{
  static const char fname[] = "capset_get_allowed_features";
  int i;

  CCAPP_DEBUG(DEB_F_PREFIX"updating idleset",
            DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  for (i=0;i < CCAPI_CALL_CAP_MAX; i++) {
      features[i] = capability_set[state][i];
  }

}











static void fcp_set_capabilities()
{
   int my_fcp_index = 0;

    if ( (fcp_index+1) >= FCP_FEATURE_MAX) {
        fcp_index = (FCP_FEATURE_MAX -1);
        CONFIG_ERROR(CFG_F_PREFIX "Received more than the maximum supported features [%d] in FCP \n", "fcp_set_capabilities", FCP_FEATURE_MAX);                    
        
    }
   
   
   for (my_fcp_index = 0; my_fcp_index <= fcp_index; my_fcp_index++)
   {   
       fcp_set_index(cc_feat_control_policy[my_fcp_index].featureId, (cc_feat_control_policy[my_fcp_index].featureEnabled == TRUE));       
   }
}   





static void fcp_init()
{
   
   fcp_index = -1;
      
   
   cc_fcp_id_to_capability_map[CALL_FORWARD_ALL_FCP_INDEX]   = CCAPI_CALL_CAP_CALLFWD;
   cc_fcp_id_to_capability_map[REDIAL_FCP_INDEX]             = CCAPI_CALL_CAP_REDIAL;

   
   capset_init();
   
   
   strcpy (g_fp_version_stamp, "");
}






static void config_parse_fcp_feature (char* featureName, xmlNode* fcp_node, xmlDocPtr doc)
{
    xmlChar*     xmlValue;
    xmlNode*     cur_node;
    
    
    if ((fcp_index+1) >= FCP_FEATURE_MAX) 
    {
        CONFIG_ERROR(CFG_F_PREFIX "Received more than the maximum supported features [%d] in FCP \n", "config_parse_fcp_feature", FCP_FEATURE_MAX);                    
        return;
    }
    fcp_index++;
    
    
    strcpy (cc_feat_control_policy[fcp_index].featureName, "");
    cc_feat_control_policy[fcp_index].featureEnabled = FALSE;
    cc_feat_control_policy[fcp_index].featureId       = 0;
    
    
    strncpy (cc_feat_control_policy[fcp_index].featureName, featureName, FCP_FEATURE_NAME_MAX);

    
    for (cur_node = fcp_node; cur_node; cur_node = cur_node->next) 
    {
        if (cur_node->type == XML_ELEMENT_NODE) 
        {
            xmlValue = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
            if (xmlValue != NULL)
            {
            
                if (!xmlStrcmp(cur_node->name, (const xmlChar *) "id")) 
                {        
                    unsigned int featureId = atoi((char*)xmlValue);
                    if (featureId > FCP_FEATURE_MAX)
                    {
                        CONFIG_ERROR(CFG_F_PREFIX "Feature Id [%d] in FCP file Out Of Range [%d] \n", "config_parse_fcp_feature", featureId, FCP_FEATURE_MAX);                    
                        featureId = 0;
                    }
            
                    cc_feat_control_policy[fcp_index].featureId = featureId;
                                                            
                } else if (!xmlStrcmp(cur_node->name, (const xmlChar *) "enable")) 
                {
                    cc_feat_control_policy[fcp_index].featureEnabled = (cc_boolean)(strcmp((char*)xmlValue, "true") == 0);                
                }
                
                xmlFree(xmlValue); 
            }        
        }
    }
}







static void config_parse_fcp_element (xmlNode* fcp_node, char* value, xmlDocPtr doc )
{    
   int versionStampSize = 0;
    
    if (!xmlStrcmp(fcp_node->name, (const xmlChar *) "versionStamp")) 
    {
        versionStampSize = strlen(value);
    
        
        if (versionStampSize > MAX_FP_VERSION_STAMP_LEN) {
            CONFIG_ERROR(CFG_F_PREFIX "FCP Version Length [%d] is crazy large!\n", "config_parse_fcp_element", versionStampSize);
            return;
        }

        
        sstrncpy(g_fp_version_stamp, value, MAX_FP_VERSION_STAMP_LEN);        

    } else if (!xmlStrcmp(fcp_node->name, (const xmlChar *) "featureDef")) 
    {
        xmlChar* featureName = NULL;
            
        featureName = xmlGetProp(fcp_node, (const xmlChar *) "name");
        if (featureName == NULL)
        {
            CONFIG_ERROR(CFG_F_PREFIX "FCP Couldnt get feature name element!\n", "config_parse_fcp_element");
            return;
        }
        
        
        CONFIG_DEBUG(CFG_F_PREFIX "FCP Parsing FeatureName=[%s] \n", "config_parse_fcp_element", (char*)featureName);
        config_parse_fcp_feature ((char*)featureName, fcp_node->children, doc);        
        
        xmlFree(featureName); 
    }     
}












static int fcp_parse_and_set (xmlNode* a_node, xmlDocPtr doc)
{    
    xmlNode*  cur_node = NULL;
    xmlChar*  data;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
    {
        if (cur_node->type == XML_ELEMENT_NODE) 
        {
            data = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
            if (data != NULL) 
            {
                config_parse_fcp_element(cur_node, (char *)data, doc);
                xmlFree(data);
            }
        }
        
        fcp_parse_and_set(cur_node->children, doc);
    }

    return (0);
}





int fcp_init_template (const char* fcp_plan_string)
{
    static const char fname[] = "fcp_init_template";
 
    xmlNode*  fcp_xml_root_element;
    xmlDocPtr fcp_xml_doc;
        
    fcp_init();    
        
    if (fcp_plan_string == NULL)
    {   
       return (0);  
    }

    fcp_xml_doc = xmlReadFile(fcp_plan_string, NULL, 0);
    if (fcp_xml_doc == NULL)
    {    
        CONFIG_ERROR(CFG_F_PREFIX "FCP - Unable to xmlReadFile = [%s]", fname, fcp_plan_string);
        return (-1);    
    }

    
    fcp_xml_root_element = xmlDocGetRootElement(fcp_xml_doc);
    if (fcp_xml_root_element == NULL)
    {    
        CONFIG_ERROR(CFG_F_PREFIX "FCP - Unable to get root element for FCP file = [%s]", fname, fcp_plan_string);
        xmlFreeDoc(fcp_xml_doc);

        return (-1);   
    }

    fcp_parse_and_set (fcp_xml_root_element, fcp_xml_doc);

    
    xmlFreeDoc(fcp_xml_doc);

    
    CONFIG_DEBUG(DEB_F_PREFIX"FCP Post FCP Parse Setting Capabilities\n", DEB_F_PREFIX_ARGS(JNI, "fcp_display"));

    
    fcp_set_capabilities();

    return (0);
}

