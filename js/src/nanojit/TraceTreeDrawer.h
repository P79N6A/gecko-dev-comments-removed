




































#ifndef __nanojit_TraceTreeDrawer__
#define __nanojit_TraceTreeDrawer__
#include <stdio.h>

namespace nanojit {
#ifdef AVMPLUS_VERBOSE
    using namespace avmplus;
    
    class TraceTreeDrawer : public GCFinalizedObject {
    public:
    	TraceTreeDrawer(Fragmento *frago, AvmCore *core, AvmString fileName);
    	~TraceTreeDrawer();
    	
    	void createGraphHeader();
    	void createGraphFooter();
    	
        void addEdge(Fragment *from, Fragment *to);
        void addNode(Fragment *fragment);
        void addNode(Fragment *fragment, const char *color);
	
        void draw(Fragment *rootFragment);
        void recursiveDraw(Fragment *root);
    	
    private:
    	FILE*				_fstream;
    	DWB(AvmCore *)		_core;
    	DWB(Fragmento *)	_frago;
    	DWB(LabelMap *)		_labels;
    	AvmString _fileName;
    	
    	void addBackEdges(Fragment *f);
    	void addMergeNode(Fragment *f);
    	
    	bool isValidSideExit(struct SideExit *exit);
    	bool isCompiled(Fragment *f);
    	bool isLoopFragment(Fragment *f);
    	bool isCrossFragment(Fragment *f);
    	bool isMergeFragment(Fragment *f);
    	bool isSingleTrace(Fragment *f);
    	bool isSpawnedTrace(Fragment *f);
    	bool isBackEdgeSideExit(Fragment *f);
    	bool hasEndOfTraceFrag(Fragment *f);
    	bool hasCompiledBranch(Fragment *f);
    	
    	void printTreeStatus(Fragment *root);
    	void drawDirectedEdge();
    };
#endif
}

#endif 
