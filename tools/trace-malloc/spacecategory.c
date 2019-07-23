

















































#include "spacetrace.h"
#include <ctype.h>
#include <string.h>




#include "nsQuickSort.h"

#if defined(HAVE_BOUTELL_GD)





#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#endif 






int
AddRule(STGlobals * g, STCategoryRule * rule)
{
    if (g->mNRules % ST_ALLOC_STEP == 0) {
        
        STCategoryRule **newrules;

        newrules = (STCategoryRule **) realloc(g->mCategoryRules,
                                               (g->mNRules +
                                                ST_ALLOC_STEP) *
                                               sizeof(STCategoryRule *));
        if (!newrules) {
            REPORT_ERROR(__LINE__, AddRule_No_Memory);
            return -1;
        }
        g->mCategoryRules = newrules;
    }
    g->mCategoryRules[g->mNRules++] = rule;
    return 0;
}






int
AddChild(STCategoryNode * parent, STCategoryNode * child)
{
    if (parent->nchildren % ST_ALLOC_STEP == 0) {
        
        STCategoryNode **newnodes;

        newnodes = (STCategoryNode **) realloc(parent->children,
                                               (parent->nchildren +
                                                ST_ALLOC_STEP) *
                                               sizeof(STCategoryNode *));
        if (!newnodes) {
            REPORT_ERROR(__LINE__, AddChild_No_Memory);
            return -1;
        }
        parent->children = newnodes;
    }
    parent->children[parent->nchildren++] = child;
    return 0;
}

int
ReParent(STCategoryNode * parent, STCategoryNode * child)
{
    PRUint32 i;

    if (child->parent == parent)
        return 0;

    
    if (child->parent) {
        for (i = 0; i < child->parent->nchildren; i++) {
            if (child->parent->children[i] == child) {
                
                if (i + 1 < child->parent->nchildren)
                    memmove(&child->parent->children[i],
                            &child->parent->children[i + 1],
                            (child->parent->nchildren - i -
                             1) * sizeof(STCategoryNode *));
                child->parent->nchildren--;
                break;
            }
        }
    }

    
    AddChild(parent, child);

    return 0;
}






STCategoryNode *
findCategoryNode(const char *catName, STGlobals * g)
{
    PRUint32 i;

    for (i = 0; i < g->mNCategoryMap; i++) {
        if (!strcmp(g->mCategoryMap[i]->categoryName, catName))
            return g->mCategoryMap[i]->node;
    }

    
    if (!strcmp(catName, ST_ROOT_CATEGORY_NAME))
        return &g->mCategoryRoot;

    return NULL;
}






int
AddCategoryNode(STCategoryNode * node, STGlobals * g)
{
    if (g->mNCategoryMap % ST_ALLOC_STEP == 0) {
        
        STCategoryMapEntry **newmap =
            (STCategoryMapEntry **) realloc(g->mCategoryMap,
                                            (g->mNCategoryMap +
                                             ST_ALLOC_STEP) *
                                            sizeof(STCategoryMapEntry *));
        if (!newmap) {
            REPORT_ERROR(__LINE__, AddCategoryNode_No_Memory);
            return -1;
        }
        g->mCategoryMap = newmap;

    }
    g->mCategoryMap[g->mNCategoryMap] =
        (STCategoryMapEntry *) calloc(1, sizeof(STCategoryMapEntry));
    if (!g->mCategoryMap[g->mNCategoryMap]) {
        REPORT_ERROR(__LINE__, AddCategoryNode_No_Memory);
        return -1;
    }
    g->mCategoryMap[g->mNCategoryMap]->categoryName = node->categoryName;
    g->mCategoryMap[g->mNCategoryMap]->node = node;
    g->mNCategoryMap++;
    return 0;
}







STCategoryNode *
NewCategoryNode(const char *catName, STCategoryNode * parent, STGlobals * g)
{
    STCategoryNode *node;

    node = (STCategoryNode *) calloc(1, sizeof(STCategoryNode));
    if (!node)
        return NULL;

    node->runs =
        (STRun **) calloc(g->mCommandLineOptions.mContexts, sizeof(STRun *));
    if (NULL == node->runs) {
        free(node);
        return NULL;
    }

    node->categoryName = catName;

    
    node->parent = parent;

    
    AddChild(parent, node);

    
    AddCategoryNode(node, g);

    return node;
}







int
ProcessCategoryLeafRule(STCategoryRule * leafRule, STCategoryNode * root,
                        STGlobals * g)
{
    STCategoryRule *rule;
    STCategoryNode *node;

    rule = (STCategoryRule *) calloc(1, sizeof(STCategoryRule));
    if (!rule)
        return -1;

    
    *rule = *leafRule;

    
    node = findCategoryNode(rule->categoryName, g);
    if (!node)
        node = NewCategoryNode(rule->categoryName, root, g);

    
    rule->node = node;

    
    AddRule(g, rule);

    return 0;
}







int
ProcessCategoryParentRule(STCategoryRule * parentRule, STCategoryNode * root,
                          STGlobals * g)
{
    STCategoryNode *node;
    STCategoryNode *child;
    PRUint32 i;

    
    node = findCategoryNode(parentRule->categoryName, g);
    if (!node) {
        node = NewCategoryNode(parentRule->categoryName, root, g);
        if (!node)
            return -1;
    }

    
    for (i = 0; i < parentRule->npats; i++) {
        child = findCategoryNode(parentRule->pats[i], g);
        if (!child) {
            child = NewCategoryNode(parentRule->pats[i], node, g);
            if (!child)
                return -1;
        }
        else {
            



            ReParent(node, child);
        }
    }

    return 0;
}








int
initCategories(STGlobals * g)
{
    FILE *fp;
    char buf[1024], *in;
    int n;
    PRBool inrule, leaf;
    STCategoryRule rule;

    fp = fopen(g->mCommandLineOptions.mCategoryFile, "r");
    if (!fp) {
        
        REPORT_INFO("No categories file.");
        return -1;
    }

    inrule = PR_FALSE;
    leaf = PR_FALSE;

    memset(&rule, 0, sizeof(rule));

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        
        n = strlen(buf);
        if (buf[n - 1] == '\n')
            buf[--n] = '\0';
        in = buf;

        
        if (*in == '#')
            continue;

        
        while (*in && isspace(*in))
            in++;
        if (*in == '\0') {
            if (inrule) {
                
                if (leaf)
                    ProcessCategoryLeafRule(&rule, &g->mCategoryRoot, g);
                else
                    
                    ProcessCategoryParentRule(&rule, &g->mCategoryRoot, g);
                inrule = PR_FALSE;
                memset(&rule, 0, sizeof(rule));
            }
            continue;
        }

        
        if (inrule) {
            rule.pats[rule.npats] = strdup(in);
            rule.patlen[rule.npats++] = strlen(in);
        }
        else if (*in == '<') {
            
            inrule = PR_TRUE;
            leaf = PR_TRUE;

            
            in++;
            n = strlen(in);
            if (in[n - 1] == '>')
                in[n - 1] = '\0';
            rule.categoryName = strdup(in);
        }
        else {
            


            inrule = PR_TRUE;
            leaf = PR_FALSE;
            rule.categoryName = strdup(in);
        }
    }

    
    if (inrule) {
        
        if (leaf)
            ProcessCategoryLeafRule(&rule, &g->mCategoryRoot, g);
        else
            
            ProcessCategoryParentRule(&rule, &g->mCategoryRoot, g);
    }

    



    memset(&rule, 0, sizeof(rule));
    rule.categoryName = strdup("uncategorized");
    rule.pats[0] = strdup("");
    rule.patlen[0] = 0;
    rule.npats = 1;
    ProcessCategoryLeafRule(&rule, &g->mCategoryRoot, g);

    return 0;
}







int
callsiteMatchesRule(tmcallsite * aCallsite, STCategoryRule * aRule)
{
    PRUint32 patnum = 0;
    const char *methodName = NULL;

    while (patnum < aRule->npats && aCallsite && aCallsite->method) {
        methodName = tmmethodnode_name(aCallsite->method);
        if (!methodName)
            return 0;
        if (!*aRule->pats[patnum]
            || !strncmp(methodName, aRule->pats[patnum],
                        aRule->patlen[patnum])) {
            
            patnum++;
            aCallsite = aCallsite->parent;
        }
        else {
            
            if (patnum > 0) {
                
                return 0;
            }
            


            aCallsite = aCallsite->parent;
        }
    }

    if (patnum == aRule->npats) {
        
#if defined(DEBUG_dp) && 0
        fprintf(stderr, "[%s] match\n", aRule->categoryName);
#endif
        return 1;
    }

    return 0;
}

#ifdef DEBUG_dp
PRIntervalTime _gMatchTime = 0;
PRUint32 _gMatchCount = 0;
PRUint32 _gMatchRules = 0;
#endif







STCategoryNode *
matchAllocation(STGlobals * g, STAllocation * aAllocation)
{
#ifdef DEBUG_dp
    PRIntervalTime start = PR_IntervalNow();
#endif
    PRUint32 rulenum;
    STCategoryNode *node = NULL;
    STCategoryRule *rule;

    for (rulenum = 0; rulenum < g->mNRules; rulenum++) {
#ifdef DEBUG_dp
        _gMatchRules++;
#endif
        rule = g->mCategoryRules[rulenum];
        if (callsiteMatchesRule(aAllocation->mEvents[0].mCallsite, rule)) {
            node = rule->node;
            break;
        }
    }
#ifdef DEBUG_dp
    _gMatchCount++;
    _gMatchTime += PR_IntervalNow() - start;
#endif
    return node;
}










int
categorizeAllocation(STOptions * inOptions, STContext * inContext,
                     STAllocation * aAllocation, STGlobals * g)
{
    


    STCategoryNode *node;

    node = matchAllocation(g, aAllocation);
    if (!node) {
        

        REPORT_ERROR(__LINE__, categorizeAllocation);
        return -1;
    }

    
    if (!node->runs[inContext->mIndex]) {
        



        node->runs[inContext->mIndex] =
            createRun(inContext, PR_IntervalNow());
        if (!node->runs[inContext->mIndex]) {
            REPORT_ERROR(__LINE__, categorizeAllocation_No_Memory);
            return -1;
        }
    }

    
    if (node->runs[inContext->mIndex]->mAllocationCount % ST_ALLOC_STEP == 0) {
        
        STAllocation **allocs;

        allocs =
            (STAllocation **) realloc(node->runs[inContext->mIndex]->
                                      mAllocations,
                                      (node->runs[inContext->mIndex]->
                                       mAllocationCount +
                                       ST_ALLOC_STEP) *
                                      sizeof(STAllocation *));
        if (!allocs) {
            REPORT_ERROR(__LINE__, categorizeAllocation_No_Memory);
            return -1;
        }
        node->runs[inContext->mIndex]->mAllocations = allocs;
    }
    node->runs[inContext->mIndex]->mAllocations[node->
                                                runs[inContext->mIndex]->
                                                mAllocationCount++] =
        aAllocation;

    




    recalculateAllocationCost(inOptions, inContext,
                              node->runs[inContext->mIndex], aAllocation,
                              PR_FALSE);

    
    
#if defined(DEBUG_dp) && 0
    fprintf(stderr, "DEBUG: [%s] match\n", node->categoryName);
#endif
    return 0;
}

typedef PRBool STCategoryNodeProcessor(STRequest * inRequest,
                                       STOptions * inOptions,
                                       STContext * inContext,
                                       void *clientData,
                                       STCategoryNode * node);

PRBool
freeNodeRunProcessor(STRequest * inRequest, STOptions * inOptions,
                     STContext * inContext, void *clientData,
                     STCategoryNode * node)
{
    if (node->runs && node->runs[inContext->mIndex]) {
        freeRun(node->runs[inContext->mIndex]);
        node->runs[inContext->mIndex] = NULL;
    }
    return PR_TRUE;
}

PRBool
freeNodeRunsProcessor(STRequest * inRequest, STOptions * inOptions,
                      STContext * inContext, void *clientData,
                      STCategoryNode * node)
{
    if (node->runs) {
        PRUint32 loop = 0;

        for (loop = 0; loop < globals.mCommandLineOptions.mContexts; loop++) {
            if (node->runs[loop]) {
                freeRun(node->runs[loop]);
                node->runs[loop] = NULL;
            }
        }

        free(node->runs);
        node->runs = NULL;
    }

    return PR_TRUE;
}

#if defined(DEBUG_dp)
PRBool
printNodeProcessor(STRequest * inRequest, STOptions * inOptions,
                   STContext * inContext, void *clientData,
                   STCategoryNode * node)
{
    STCategoryNode *root = (STCategoryNode *) clientData;

    fprintf(stderr, "%-25s [ %9s size", node->categoryName,
            FormatNumber(node->run ? node->run->mStats[inContext->mIndex].
                         mSize : 0));
    fprintf(stderr, ", %5.1f%%",
            node->run ? ((double) node->run->mStats[inContext->mIndex].mSize /
                         root->run->mStats[inContext->mIndex].mSize *
                         100) : 0);
    fprintf(stderr, ", %7s allocations ]\n",
            FormatNumber(node->run ? node->run->mStats[inContext->mIndex].
                         mCompositeCount : 0));
    return PR_TRUE;
}

#endif

typedef struct __struct_optcon
{
    STOptions *mOptions;
    STContext *mContext;
}
optcon;







int
compareNode(const void *aNode1, const void *aNode2, void *aContext)
{
    int retval = 0;
    STCategoryNode *node1, *node2;
    PRUint32 a, b;
    optcon *oc = (optcon *) aContext;

    if (!aNode1 || !aNode2 || !oc->mOptions || !oc->mContext)
        return 0;

    node1 = *((STCategoryNode **) aNode1);
    node2 = *((STCategoryNode **) aNode2);

    if (node1 && node2) {
        if (oc->mOptions->mOrderBy == ST_COUNT) {
            a = (node1->runs[oc->mContext->mIndex]) ? node1->runs[oc->
                                                                  mContext->
                                                                  mIndex]->
                mStats[oc->mContext->mIndex].mCompositeCount : 0;
            b = (node2->runs[oc->mContext->mIndex]) ? node2->runs[oc->
                                                                  mContext->
                                                                  mIndex]->
                mStats[oc->mContext->mIndex].mCompositeCount : 0;
        }
        else {
            
            a = (node1->runs[oc->mContext->mIndex]) ? node1->runs[oc->
                                                                  mContext->
                                                                  mIndex]->
                mStats[oc->mContext->mIndex].mSize : 0;
            b = (node2->runs[oc->mContext->mIndex]) ? node2->runs[oc->
                                                                  mContext->
                                                                  mIndex]->
                mStats[oc->mContext->mIndex].mSize : 0;
        }
        if (a < b)
            retval = __LINE__;
        else
            retval = -__LINE__;
    }
    return retval;
}

PRBool
sortNodeProcessor(STRequest * inRequest, STOptions * inOptions,
                  STContext * inContext, void *clientData,
                  STCategoryNode * node)
{
    if (node->nchildren) {
        optcon context;

        context.mOptions = inOptions;
        context.mContext = inContext;

        NS_QuickSort(node->children, node->nchildren,
                     sizeof(STCategoryNode *), compareNode, &context);
    }

    return PR_TRUE;
}









#define MODINC(n, mod) ((n+1) % mod)

void
walkTree(STCategoryNode * root, STCategoryNodeProcessor func,
         STRequest * inRequest, STOptions * inOptions, STContext * inContext,
         void *clientData, int maxdepth)
{
    STCategoryNode *nodes[1024], *node;
    PRUint32 begin, end, i;
    int ret = 0;
    int curdepth = 0, ncurdepth = 0;

    nodes[0] = root;
    begin = 0;
    end = 1;
    ncurdepth = 1;
    while (begin != end) {
        node = nodes[begin];
        ret = (*func) (inRequest, inOptions, inContext, clientData, node);
        if (ret == PR_FALSE) {
            
            break;
        }
        begin = MODINC(begin, 1024);
        for (i = 0; i < node->nchildren; i++) {
            nodes[end] = node->children[i];
            end = MODINC(end, 1024);
        }
        
        if (maxdepth > 0 && --ncurdepth == 0) {
            



            ncurdepth = (begin < end) ? (end - begin) : (1024 - begin + end);
            if (++curdepth > maxdepth) {
                


                break;
            }
        }
    }
    return;
}

int
freeRule(STCategoryRule * rule)
{
    PRUint32 i;
    char *p = (char *) rule->categoryName;

    PR_FREEIF(p);

    for (i = 0; i < rule->npats; i++)
        free(rule->pats[i]);

    free(rule);
    return 0;
}

void
freeNodeRuns(STCategoryNode * root)
{
    walkTree(root, freeNodeRunsProcessor, NULL, NULL, NULL, NULL, 0);
}

void
freeNodeMap(STGlobals * g)
{
    PRUint32 i;

    
    for (i = 0; i < g->mNCategoryMap; i++) {
        free(g->mCategoryMap[i]->node);
        free(g->mCategoryMap[i]);
    }
    free(g->mCategoryMap);
}

int
freeCategories(STGlobals * g)
{
    PRUint32 i;

    


    freeNodeRuns(&g->mCategoryRoot);

    


    freeNodeMap(g);

    


    for (i = 0; i < g->mNRules; i++) {
        freeRule(g->mCategoryRules[i]);
    }
    free(g->mCategoryRules);

    return 0;
}








int
categorizeRun(STOptions * inOptions, STContext * inContext,
              const STRun * aRun, STGlobals * g)
{
    PRUint32 i;

#if defined(DEBUG_dp)
    PRIntervalTime start = PR_IntervalNow();

    fprintf(stderr, "DEBUG: categorizing run...\n");
#endif

    


    walkTree(&g->mCategoryRoot, freeNodeRunProcessor, NULL, inOptions,
             inContext, NULL, 0);

    if (g->mNCategoryMap > 0) {
        for (i = 0; i < aRun->mAllocationCount; i++) {
            categorizeAllocation(inOptions, inContext, aRun->mAllocations[i],
                                 g);
        }
    }

    


    g->mCategoryRoot.runs[inContext->mIndex] = (STRun *) aRun;
    g->mCategoryRoot.categoryName = ST_ROOT_CATEGORY_NAME;

#if defined(DEBUG_dp)
    fprintf(stderr,
            "DEBUG: categorizing ends: %dms [%d rules, %d allocations]\n",
            PR_IntervalToMilliseconds(PR_IntervalNow() - start), g->mNRules,
            aRun->mAllocationCount);
    fprintf(stderr, "DEBUG: match : %dms [%d calls, %d rule-compares]\n",
            PR_IntervalToMilliseconds(_gMatchTime), _gMatchCount,
            _gMatchRules);
#endif

    


    walkTree(&g->mCategoryRoot, sortNodeProcessor, NULL, inOptions, inContext,
             NULL, 0);

#if defined(DEBUG_dp)
    walkTree(&g->mCategoryRoot, printNodeProcessor, NULL, inOptions,
             inContext, &g->mCategoryRoot, 0);
#endif

    return 0;
}








PRBool
displayCategoryNodeProcessor(STRequest * inRequest, STOptions * inOptions,
                             STContext * inContext, void *clientData,
                             STCategoryNode * node)
{
    STCategoryNode *root = (STCategoryNode *) clientData;
    PRUint32 byteSize = 0, heapCost = 0, count = 0;
    double percent = 0;
    STOptions customOps;

    if (node->runs[inContext->mIndex]) {
        


        byteSize =
            node->runs[inContext->mIndex]->mStats[inContext->mIndex].mSize;

        


        count =
            node->runs[inContext->mIndex]->mStats[inContext->mIndex].
            mCompositeCount;

        


        heapCost =
            node->runs[inContext->mIndex]->mStats[inContext->mIndex].
            mHeapRuntimeCost;

        


        if (root->runs[inContext->mIndex]) {
            percent =
                ((double) byteSize) /
                root->runs[inContext->mIndex]->mStats[inContext->mIndex].
                mSize * 100;
        }
    }

    PR_fprintf(inRequest->mFD, " <tr>\n" "  <td>");

    
    memcpy(&customOps, inOptions, sizeof(customOps));
    PR_snprintf(customOps.mCategoryName, sizeof(customOps.mCategoryName),
                "%s", node->categoryName);

    htmlAnchor(inRequest, "top_callsites.html", node->categoryName, NULL,
               "category-callsites", &customOps);
    PR_fprintf(inRequest->mFD,
               "</td>\n" "  <td align=right>%u</td>\n"
               "  <td align=right>%4.1f%%</td>\n"
               "  <td align=right>%u</td>\n" "  <td align=right>"
               ST_MICROVAL_FORMAT "</td>\n" " </tr>\n", byteSize, percent,
               count, ST_MICROVAL_PRINTABLE(heapCost));

    return PR_TRUE;
}


int
displayCategoryReport(STRequest * inRequest, STCategoryNode * root, int depth)
{
    PR_fprintf(inRequest->mFD,
               "<table class=\"category-list data\">\n"
               " <tr class=\"row-header\">\n"
               "  <th>Category</th>\n"
               "  <th>Composite Byte Size</th>\n"
               "  <th>%% of Total Size</th>\n"
               "  <th>Heap Object Count</th>\n"
               "  <th>Composite Heap Operations Seconds</th>\n" " </tr>\n");

    walkTree(root, displayCategoryNodeProcessor, inRequest,
             &inRequest->mOptions, inRequest->mContext, root, depth);

    PR_fprintf(inRequest->mFD, "</table>\n");

    return 0;
}
