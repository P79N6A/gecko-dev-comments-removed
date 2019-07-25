









#include "MatlabPlot.h"
#ifdef MATLAB
#include "engine.h"
#endif
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "critical_section_wrapper.h"
#include "tick_util.h"

#include <sstream>
#include <algorithm>
#include <math.h>
#include <stdio.h>

using namespace webrtc;

#ifdef MATLAB
MatlabEngine eng;

MatlabLine::MatlabLine(int maxLen , const char *plotAttrib , const char *name )
:
_xArray(NULL),
_yArray(NULL),
_maxLen(maxLen),
_plotAttribute(),
_name()
{
    if (_maxLen > 0)
    {
        _xArray = mxCreateDoubleMatrix(1, _maxLen, mxREAL);
        _yArray = mxCreateDoubleMatrix(1, _maxLen, mxREAL);
    }

    if (plotAttrib)
    {
        _plotAttribute = plotAttrib;
    }

    if (name)
    {
        _name = name;
    }
}

MatlabLine::~MatlabLine()
{
    if (_xArray != NULL)
    {
        mxDestroyArray(_xArray);
    }
    if (_yArray != NULL)
    {
        mxDestroyArray(_yArray);
    }
}

void MatlabLine::Append(double x, double y)
{
    if (_maxLen > 0 && _xData.size() > static_cast<WebRtc_UWord32>(_maxLen))
    {
        _xData.resize(_maxLen);
        _yData.resize(_maxLen);
    }

    _xData.push_front(x);
    _yData.push_front(y);
}



void MatlabLine::Append(double y)
{
    if (_xData.empty())
    {
        
        Append(0, y);
    }
    else
    {
        
        double temp = _xData.back(); 
        Append(temp + 1, y);
    }
}


void MatlabLine::SetMaxLen(int maxLen)
{
    if (maxLen <= 0)
    {
        
        _maxLen = -1;
    }
    else
    {
        _maxLen = maxLen;

        if (_xArray != NULL)
        {
            mxDestroyArray(_xArray);
            mxDestroyArray(_yArray);
        }
        _xArray = mxCreateDoubleMatrix(1, _maxLen, mxREAL);
        _yArray = mxCreateDoubleMatrix(1, _maxLen, mxREAL);

        maxLen = ((unsigned int)maxLen <= _xData.size()) ? maxLen : (int)_xData.size();
        _xData.resize(maxLen);
        _yData.resize(maxLen);

        
        
        
    }
}

void MatlabLine::SetAttribute(char *plotAttrib)
{
    _plotAttribute = plotAttrib;
}

void MatlabLine::SetName(char *name)
{
    _name = name;
}

void MatlabLine::GetPlotData(mxArray** xData, mxArray** yData)
{
    
    
    if (_xData.empty())
    {
        return; 
    }
    unsigned int size = 0;
    if (_xArray != NULL)
    {
        size = (unsigned int)mxGetNumberOfElements(_xArray);
    }
    if (size < _xData.size())
    {
        if (_xArray != NULL)
        {
            mxDestroyArray(_xArray);
            mxDestroyArray(_yArray);
        }
        _xArray = mxCreateDoubleMatrix(1, _xData.size(), mxREAL);
        _yArray = mxCreateDoubleMatrix(1, _yData.size(), mxREAL);
    }

    if (!_xData.empty())
    {
        double* x = mxGetPr(_xArray);

        std::list<double>::iterator it = _xData.begin();

        for (int i = 0; it != _xData.end(); it++, i++)
        {
            x[i] = *it;
        }
    }

    if (!_yData.empty())
    {
        double* y = mxGetPr(_yArray);

        std::list<double>::iterator it = _yData.begin();

        for (int i = 0; it != _yData.end(); it++, i++)
        {
            y[i] = *it;
        }
    }
    *xData = _xArray;
    *yData = _yArray;
}

std::string MatlabLine::GetXName()
{
    std::ostringstream xString;
    xString << "x_" << _name;
    return xString.str();
}

std::string MatlabLine::GetYName()
{
    std::ostringstream yString;
    yString << "y_" << _name;
    return yString.str();
}

std::string MatlabLine::GetPlotString()
{

    std::ostringstream s;

    if (_xData.size() == 0)
    {
        s << "[0 1], [0 1]"; 
    }
    else
    {
        s << GetXName() << "(1:" << _xData.size() << "),";
        s << GetYName() << "(1:" << _yData.size() << ")";
    }

    s << ", '";
    s << _plotAttribute;
    s << "'";

    return s.str();
}

std::string MatlabLine::GetRefreshString()
{
    std::ostringstream s;

    if (_xData.size() > 0)
    {
        s << "set(h,'xdata',"<< GetXName() <<"(1:" << _xData.size() << "),'ydata',"<< GetYName() << "(1:" << _yData.size() << "));";
    }
    else
    {
        s << "set(h,'xdata',[NaN],'ydata',[NaN]);";
    }
    return s.str();
}

std::string MatlabLine::GetLegendString()
{
    return ("'" + _name + "'");
}

bool MatlabLine::hasLegend()
{
    return (!_name.empty());
}



void MatlabLine::Reset()
{
    _xData.clear();
    _yData.clear();
}


void MatlabLine::UpdateTrendLine(MatlabLine * sourceData, double slope, double offset)
{
    Reset(); 

    double thexMin = sourceData->xMin();
    double thexMax = sourceData->xMax();
    Append(thexMin, thexMin * slope + offset);
    Append(thexMax, thexMax * slope + offset);
}

double MatlabLine::xMin()
{
    if (!_xData.empty())
    {
        std::list<double>::iterator theStart = _xData.begin();
        std::list<double>::iterator theEnd = _xData.end();
        return(*min_element(theStart, theEnd));
    }
    return (0.0);
}

double MatlabLine::xMax()
{
    if (!_xData.empty())
    {
        std::list<double>::iterator theStart = _xData.begin();
        std::list<double>::iterator theEnd = _xData.end();
        return(*max_element(theStart, theEnd));
    }
    return (0.0);
}

double MatlabLine::yMin()
{
    if (!_yData.empty())
    {
        std::list<double>::iterator theStart = _yData.begin();
        std::list<double>::iterator theEnd = _yData.end();
        return(*min_element(theStart, theEnd));
    }
    return (0.0);
}

double MatlabLine::yMax()
{
    if (!_yData.empty())
    {
        std::list<double>::iterator theStart = _yData.begin();
        std::list<double>::iterator theEnd = _yData.end();
        return(*max_element(theStart, theEnd));
    }
    return (0.0);
}



MatlabTimeLine::MatlabTimeLine(int horizonSeconds , const char *plotAttrib ,
                               const char *name ,
                               WebRtc_Word64 refTimeMs )
                               :
_timeHorizon(horizonSeconds),
MatlabLine(-1, plotAttrib, name) 
{
    if (refTimeMs < 0)
        _refTimeMs = TickTime::MillisecondTimestamp();
    else
        _refTimeMs = refTimeMs;
}

void MatlabTimeLine::Append(double y)
{
    MatlabLine::Append(static_cast<double>(TickTime::MillisecondTimestamp() - _refTimeMs) / 1000.0, y);

    PurgeOldData();
}


void MatlabTimeLine::PurgeOldData()
{
    if (_timeHorizon > 0)
    {
        
        double historyLimit = static_cast<double>(TickTime::MillisecondTimestamp() - _refTimeMs) / 1000.0
            - _timeHorizon; 

        std::list<double>::reverse_iterator ritx = _xData.rbegin();
        WebRtc_UWord32 removeCount = 0;
        while (ritx != _xData.rend())
        {
            if (*ritx >= historyLimit)
            {
                break;
            }
            ritx++;
            removeCount++;
        }
        if (removeCount == 0)
        {
            return;
        }

        
        
        
        
        
        _xData.resize(_xData.size() - removeCount);
        _yData.resize(_yData.size() - removeCount);
    }
}


WebRtc_Word64 MatlabTimeLine::GetRefTime()
{
    return(_refTimeMs);
}




MatlabPlot::MatlabPlot()
:
_figHandle(-1),
_smartAxis(false),
_critSect(CriticalSectionWrapper::CreateCriticalSection()),
_timeToPlot(false),
_plotting(false),
_enabled(true),
_firstPlot(true),
_legendEnabled(true),
_donePlottingEvent(EventWrapper::Create())
{
    CriticalSectionScoped cs(_critSect);

    _xlim[0] = 0;
    _xlim[1] = 0;
    _ylim[0] = 0;
    _ylim[1] = 0;

#ifdef PLOT_TESTING
    _plotStartTime = -1;
    _plotDelay = 0;
#endif

}


MatlabPlot::~MatlabPlot()
{
    _critSect->Enter();

    
    while (!_line.empty())
    {
        delete *(_line.end() - 1);
        _line.pop_back();
    }

    delete _critSect;
    delete _donePlottingEvent;
}


int MatlabPlot::AddLine(int maxLen , const char *plotAttrib , const char *name )
{
    CriticalSectionScoped cs(_critSect);
    if (!_enabled)
    {
        return -1;
    }

    MatlabLine *newLine = new MatlabLine(maxLen, plotAttrib, name);
    _line.push_back(newLine);

    return (static_cast<int>(_line.size() - 1)); 
}


int MatlabPlot::AddTimeLine(int maxLen , const char *plotAttrib , const char *name ,
                            WebRtc_Word64 refTimeMs )
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return -1;
    }

    MatlabTimeLine *newLine = new MatlabTimeLine(maxLen, plotAttrib, name, refTimeMs);
    _line.push_back(newLine);

    return (static_cast<int>(_line.size() - 1)); 
}


int MatlabPlot::GetLineIx(const char *name)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return -1;
    }

    
    std::vector<MatlabLine*>::iterator it = _line.begin();
    bool matchFound = false;
    int lineIx = 0;

    for (; it != _line.end(); it++, lineIx++)
    {
        if ((*it)->_name == name)
        {
            matchFound = true;
            break;
        }
    }

    if (matchFound)
    {
        return (lineIx);
    }
    else
    {
        return (-1);
    }
}


void MatlabPlot::Append(int lineIndex, double x, double y)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return;
    }

    
    if (lineIndex < 0 || lineIndex >= static_cast<int>(_line.size()))
    {
        throw "Line index out of range";
        exit(1);
    }

    return (_line[lineIndex]->Append(x, y));
}


void MatlabPlot::Append(int lineIndex, double y)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return;
    }

    
    if (lineIndex < 0 || lineIndex >= static_cast<int>(_line.size()))
    {
        throw "Line index out of range";
        exit(1);
    }

    return (_line[lineIndex]->Append(y));
}


int MatlabPlot::Append(const char *name, double x, double y)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return -1;
    }

    
    int lineIx = GetLineIx(name);

    if (lineIx < 0) 
    {
        
        lineIx = AddLine(-1, NULL, name);
    }

    
    Append(lineIx, x, y);
    return (lineIx);
}

int MatlabPlot::Append(const char *name, double y)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return -1;
    }

    
    int lineIx = GetLineIx(name);

    if (lineIx < 0) 
    {
        
        lineIx = AddLine(-1, NULL, name);
    }

    
    Append(lineIx, y);
    return (lineIx);
}

int MatlabPlot::Length(char *name)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return -1;
    }

    int ix = GetLineIx(name);
    if (ix >= 0)
    {
        return (static_cast<int>(_line[ix]->_xData.size()));
    }
    else
    {
        return (-1);
    }
}


void MatlabPlot::SetPlotAttribute(char *name, char *plotAttrib)
{
    CriticalSectionScoped cs(_critSect);

    if (!_enabled)
    {
        return;
    }

    int lineIx = GetLineIx(name);

    if (lineIx >= 0)
    {
        _line[lineIx]->SetAttribute(plotAttrib);
    }
}


void MatlabPlot::UpdateData(Engine* ep)
{
    if (!_enabled)
    {
        return;
    }

    for (std::vector<MatlabLine*>::iterator it = _line.begin(); it != _line.end(); it++)
    {
        mxArray* xData = NULL;
        mxArray* yData = NULL;
        (*it)->GetPlotData(&xData, &yData);
        if (xData != NULL)
        {
            std::string xName = (*it)->GetXName();
            std::string yName = (*it)->GetYName();
            _critSect->Leave();
#ifdef MATLAB6
            mxSetName(xData, xName.c_str());
            mxSetName(yData, yName.c_str());
            engPutArray(ep, xData);
            engPutArray(ep, yData);
#else
            int ret = engPutVariable(ep, xName.c_str(), xData);
            assert(ret == 0);
            ret = engPutVariable(ep, yName.c_str(), yData);
            assert(ret == 0);
#endif
            _critSect->Enter();
        }
    }
}

bool MatlabPlot::GetPlotCmd(std::ostringstream & cmd, Engine* ep)
{
    _critSect->Enter();

    if (!DataAvailable())
    {
        return false;
    }

    if (_firstPlot)
    {
        GetPlotCmd(cmd);
        _firstPlot = false;
    }
    else
    {
        GetRefreshCmd(cmd);
    }

    UpdateData(ep);

    _critSect->Leave();

    return true;
}


void MatlabPlot::GetPlotCmd(std::ostringstream & cmd)
{
    
    
    cmd.str(""); 

    cmd << "figure; h" << _figHandle << "= plot(";

    
    std::vector<MatlabLine*>::iterator it = _line.begin();
    cmd << (*it)->GetPlotString();

    it++;

    
    for (; it != _line.end(); it++)
    {
        cmd << ", ";
        cmd << (*it)->GetPlotString();
    }

    cmd << "); ";

    if (_legendEnabled)
    {
        GetLegendCmd(cmd);
    }

    if (_smartAxis)
    {
        double xMin = _xlim[0];
        double xMax = _xlim[1];
        double yMax = _ylim[1];
        for (std::vector<MatlabLine*>::iterator it = _line.begin(); it != _line.end(); it++)
        {
            xMax = std::max(xMax, (*it)->xMax());
            xMin = std::min(xMin, (*it)->xMin());

            yMax = std::max(yMax, (*it)->yMax());
            yMax = std::max(yMax, fabs((*it)->yMin()));
        }
        _xlim[0] = xMin;
        _xlim[1] = xMax;
        _ylim[0] = -yMax;
        _ylim[1] = yMax;

        cmd << "axis([" << _xlim[0] << ", " << _xlim[1] << ", " << _ylim[0] << ", " << _ylim[1] << "]);";
    }

    int i=1;
    for (it = _line.begin(); it != _line.end(); i++, it++)
    {
        cmd << "set(h" << _figHandle << "(" << i << "), 'Tag', " << (*it)->GetLegendString() << ");";
    }
}


void MatlabPlot::GetRefreshCmd(std::ostringstream & cmd)
{
    cmd.str(""); 
    std::vector<MatlabLine*>::iterator it = _line.begin();
    for (it = _line.begin(); it != _line.end(); it++)
    {
        cmd << "h = findobj(0, 'Tag', " << (*it)->GetLegendString() << ");";
        cmd << (*it)->GetRefreshString();
    }
    
    
    
    
}

void MatlabPlot::GetLegendCmd(std::ostringstream & cmd)
{
    std::vector<MatlabLine*>::iterator it = _line.begin();
    bool anyLegend = false;
    for (; it != _line.end(); it++)
    {
        anyLegend = anyLegend || (*it)->hasLegend();
    }
    if (anyLegend)
    {
        

        cmd << "legend(h" << _figHandle << ",{";


        
        int i = 0;
        for (std::vector<MatlabLine*>::iterator it = _line.begin(); it != _line.end(); it++)
        {
            if (i > 0)
            {
                cmd << ", ";
            }
            cmd << (*it)->GetLegendString();
            i++;
        }

        cmd << "}, 2); "; 
    }
}


bool MatlabPlot::DataAvailable()
{
    if (!_enabled)
    {
        return false;
    }

    for (std::vector<MatlabLine*>::iterator it = _line.begin(); it != _line.end(); it++)
    {
        (*it)->PurgeOldData();
    }

    return true;
}

void MatlabPlot::Plot()
{
    CriticalSectionScoped cs(_critSect);

    _timeToPlot = true;

#ifdef PLOT_TESTING
    _plotStartTime = TickTime::MillisecondTimestamp();
#endif
}


void MatlabPlot::Reset()
{
    CriticalSectionScoped cs(_critSect);

    _enabled = true;

    for (std::vector<MatlabLine*>::iterator it = _line.begin(); it != _line.end(); it++)
    {
        (*it)->Reset();
    }

}

void MatlabPlot::SetFigHandle(int handle)
{
    CriticalSectionScoped cs(_critSect);

    if (handle > 0)
        _figHandle = handle;
}

bool
MatlabPlot::TimeToPlot()
{
    CriticalSectionScoped cs(_critSect);
    return _enabled && _timeToPlot;
}

void
MatlabPlot::Plotting()
{
    CriticalSectionScoped cs(_critSect);
    _plotting = true;
}

void
MatlabPlot::DonePlotting()
{
    CriticalSectionScoped cs(_critSect);
    _timeToPlot = false;
    _plotting = false;
    _donePlottingEvent->Set();
}

void
MatlabPlot::DisablePlot()
{
    _critSect->Enter();
    while (_plotting)
    {
        _critSect->Leave();
        _donePlottingEvent->Wait(WEBRTC_EVENT_INFINITE);
        _critSect->Enter();
    }
    _enabled = false;
}

int MatlabPlot::MakeTrend(const char *sourceName, const char *trendName, double slope, double offset, const char *plotAttrib)
{
    CriticalSectionScoped cs(_critSect);

    int sourceIx;
    int trendIx;

    sourceIx = GetLineIx(sourceName);
    if (sourceIx < 0)
    {
        
        return (-1);
    }

    trendIx = GetLineIx(trendName);
    if (trendIx < 0)
    {
        
        trendIx = AddLine(2 , plotAttrib, trendName);
    }

    _line[trendIx]->UpdateTrendLine(_line[sourceIx], slope, offset);

    return (trendIx);

}


MatlabEngine::MatlabEngine()
:
_critSect(CriticalSectionWrapper::CreateCriticalSection()),
_eventPtr(NULL),
_plotThread(NULL),
_running(false),
_numPlots(0)
{
    _eventPtr = EventWrapper::Create();

    _plotThread = ThreadWrapper::CreateThread(MatlabEngine::PlotThread, this, kLowPriority, "MatlabPlot");

    if (_plotThread == NULL)
    {
        throw "Unable to start MatlabEngine thread";
        exit(1);
    }

    _running = true;

    unsigned int tid;
    _plotThread->Start(tid);

}

MatlabEngine::~MatlabEngine()
{
    _critSect->Enter();

    if (_plotThread)
    {
        _plotThread->SetNotAlive();
        _running = false;
        _eventPtr->Set();

        while (!_plotThread->Stop())
        {
            ;
        }

        delete _plotThread;
    }

    _plots.clear();

    _plotThread = NULL;

    delete _eventPtr;
    _eventPtr = NULL;

    _critSect->Leave();
    delete _critSect;

}

MatlabPlot * MatlabEngine::NewPlot(MatlabPlot *newPlot)
{
    CriticalSectionScoped cs(_critSect);

    

    if (newPlot)
    {
        newPlot->SetFigHandle(++_numPlots); 
        _plots.push_back(newPlot);
    }

    return (newPlot);

}


void MatlabEngine::DeletePlot(MatlabPlot *plot)
{
    CriticalSectionScoped cs(_critSect);

    if (plot == NULL)
    {
        return;
    }

    std::vector<MatlabPlot *>::iterator it;
    for (it = _plots.begin(); it < _plots.end(); it++)
    {
        if (plot == *it)
        {
            break;
        }
    }

    assert (plot == *it);

    (*it)->DisablePlot();

    _plots.erase(it);
    --_numPlots;

    delete plot;
}


bool MatlabEngine::PlotThread(void *obj)
{
    if (!obj)
    {
        return (false);
    }

    MatlabEngine *eng = (MatlabEngine *) obj;

    Engine *ep = engOpen(NULL);
    if (!ep)
    {
        throw "Cannot open Matlab engine";
        return (false);
    }

    engSetVisible(ep, true);
    engEvalString(ep, "close all;");

    while (eng->_running)
    {
        eng->_critSect->Enter();

        
        for (unsigned int ix = 0; ix < eng->_plots.size(); ix++)
        {
            MatlabPlot *plot = eng->_plots[ix];
            if (plot->TimeToPlot())
            {
                plot->Plotting();
                eng->_critSect->Leave();
                std::ostringstream cmd;

                if (engEvalString(ep, cmd.str().c_str()))
                {
                    
                    return (false);
                }

                
                cmd.str(""); 
                if (plot->GetPlotCmd(cmd, ep))
                {
                    
                    plot->DonePlotting();

                    WebRtc_Word64 start = TickTime::MillisecondTimestamp();
                    
                    int ret = engEvalString(ep, cmd.str().c_str());
                    printf("time=%I64i\n", TickTime::MillisecondTimestamp() - start);
                    if (ret)
                    {
                        
                        return (false);
                    }

#ifdef PLOT_TESTING
                    if(plot->_plotStartTime >= 0)
                    {
                        plot->_plotDelay = TickTime::MillisecondTimestamp() - plot->_plotStartTime;
                        plot->_plotStartTime = -1;
                    }
#endif
                }
                eng->_critSect->Enter();
            }
        }

        eng->_critSect->Leave();
        
        eng->_eventPtr->Wait(66); 
    }

    if (ep)
    {
        engClose(ep);
        ep = NULL;
    }

    return (true);

}

#endif 
