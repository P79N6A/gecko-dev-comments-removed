









#include "udp_socket_manager_windows.h"
#include "udp_socket_windows.h"

namespace webrtc {
WebRtc_UWord32 UdpSocketManagerWindows::_numOfActiveManagers = 0;

UdpSocketManagerWindows::UdpSocketManagerWindows()
    : UdpSocketManager(),
      _id(-1)
{
    const char* threadName = "UdpSocketManagerWindows_Thread";
    _critSectList = CriticalSectionWrapper::CreateCriticalSection();
    _thread = ThreadWrapper::CreateThread(UdpSocketManagerWindows::Run,
                                          this, kRealtimePriority, threadName);
    FD_ZERO(&_readFds);
    FD_ZERO(&_writeFds);
    FD_ZERO(&_exceptFds);
    _numOfActiveManagers++;
}

bool UdpSocketManagerWindows::Init(WebRtc_Word32 id,
                                   WebRtc_UWord8& numOfWorkThreads) {
    CriticalSectionScoped cs(_critSectList);
    if ((_id != -1) || (_numOfWorkThreads != 0)) {
        assert(_id == -1);
        assert(_numOfWorkThreads == 0);
        return false;
    }
    _id = id;
    _numOfWorkThreads = numOfWorkThreads;
    return true;
}

UdpSocketManagerWindows::~UdpSocketManagerWindows()
{
    Stop();
    if(_thread != NULL)
    {
        delete _thread;
    }

    if (_critSectList != NULL)
    {
        _critSectList->Enter();

        while(!_socketMap.empty())
        {
            std::map<SOCKET, UdpSocketWindows*>::iterator it =
                _socketMap.begin();
            UdpSocketWindows* s = static_cast<UdpSocketWindows*>(it->second);
            _socketMap.erase(it);
            delete s;
        }
        _removeList.erase(_removeList.begin(), _removeList.end());

        while(!_addList.empty())
        {
            std::list<UdpSocketWindows*>::iterator it = _addList.begin();
            UdpSocketWindows* s = static_cast<UdpSocketWindows*>(*it);
            _addList.erase(it);
            delete s;
        }
        _critSectList->Leave();

        delete _critSectList;
    }

    _numOfActiveManagers--;

    if (_numOfActiveManagers == 0)
        WSACleanup();
}

WebRtc_Word32 UdpSocketManagerWindows::ChangeUniqueId(const WebRtc_Word32 id)
{
    _id = id;
    return 0;
}

bool UdpSocketManagerWindows::Start()
{
    unsigned int id;
    if (_thread == NULL)
        return false;

    return _thread->Start(id);
}

bool UdpSocketManagerWindows::Stop()
{
    if (_thread == NULL)
        return true;

    return _thread->Stop();
}

bool UdpSocketManagerWindows::Process()
{
    bool doSelect = false;
    
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    FD_ZERO(&_readFds);
    FD_ZERO(&_writeFds);
    FD_ZERO(&_exceptFds);

    _critSectList->Enter();
    
    while(!_removeList.empty())
    {
        SOCKET id = *_removeList.begin();
        std::map<SOCKET, UdpSocketWindows*>::iterator it = _socketMap.find(id);
        if(it != _socketMap.end())
        {
            UdpSocketWindows* s = static_cast<UdpSocketWindows*>(it->second);
            _socketMap.erase(it);
            _removeList.pop_front();
            delete s;
        }
    }

    
    while (!_addList.empty())
    {
        UdpSocketWindows* s = *_addList.begin();
        if(s)
        {
            _socketMap[s->GetFd()] = s;
        }
        _addList.pop_front();
    }
    _critSectList->Leave();

    std::map<SOCKET, UdpSocketWindows*>::iterator it = _socketMap.begin();
    while(it != _socketMap.end())
    {
        UdpSocketWindows* s = it->second;
        if (s->WantsIncoming())
        {
            doSelect = true;
            FD_SET(it->first, &_readFds);
        }
        if(!s->IsWritable())
        {
            FD_SET(it->first, &_writeFds);
            doSelect = true;
        }
        it++;
    }

    WebRtc_Word32 num = 0;
    if (doSelect)
    {
        num = select(0, &_readFds, &_writeFds, &_exceptFds, &timeout);
        if (num == SOCKET_ERROR)
        {
            Sleep(10);
            return true;
        }
    }else
    {
        Sleep(10);
        return true;
    }

    it = _socketMap.begin();
    while (it != _socketMap.end() && num > 0)
    {
        if (FD_ISSET(it->first, &_readFds))
        {
            static_cast<UdpSocketWindows*>(it->second)->HasIncoming();
            num--;
        }
        if (FD_ISSET(it->first, &_writeFds))
        {
            
            static_cast<UdpSocketWindows*>(it->second)->SetWritable();
            num--;
        }
    }
    return true;
}

bool UdpSocketManagerWindows::Run(ThreadObj obj)
{
    UdpSocketManagerWindows* mgr = static_cast<UdpSocketManagerWindows*>(obj);
    return mgr->Process();
};

bool UdpSocketManagerWindows::AddSocket(UdpSocketWrapper* s)
{
    UdpSocketWindows* winSock = static_cast<UdpSocketWindows*>(s);

    _critSectList->Enter();
    std::map<SOCKET, UdpSocketWindows*>::iterator it =
        _socketMap.find(winSock->GetFd());
    if (it != _socketMap.end())
    {
        if (!_removeList.empty())
        {
            
            
            
            
            
            
            std::list<SOCKET>::iterator removeIt = _removeList.begin();
            while(removeIt != _removeList.end())
            {
                if (*removeIt == winSock->GetFd())
                {
                    it = _socketMap.find(*removeIt);
                    UdpSocketWindows* delete_socket = it->second;
                    _socketMap.erase(it);
                    _removeList.erase(removeIt);
                    delete delete_socket;
                    _addList.push_back(winSock);
                    _critSectList->Leave();
                    return true;
                }
                removeIt++;
            }
        }
        _critSectList->Leave();
        return false;
    }

    _addList.push_back(winSock);
    _critSectList->Leave();
    return true;
}

bool UdpSocketManagerWindows::RemoveSocket(UdpSocketWrapper* s)
{
    UdpSocketWindows* winSock = static_cast<UdpSocketWindows*>(s);

    _critSectList->Enter();
    
    if (!_addList.empty())
    {
        std::list<UdpSocketWindows*>::iterator it = _addList.begin();
        while(it != _addList.end())
        {
            UdpSocketWindows* tempSocket = (*it);
            if (tempSocket->GetFd() == winSock->GetFd())
            {
                _addList.erase(it);
                delete winSock;
                _critSectList->Leave();
                return true;
            }
            it++;
        }
    }

    
    
    std::map<SOCKET, UdpSocketWindows*>::iterator findIt =
        _socketMap.find(winSock->GetFd());
    if (findIt == _socketMap.end())
    {
        delete winSock;
        _critSectList->Leave();
        return false;
    }

    _removeList.push_back(winSock->GetFd());
    _critSectList->Leave();
    return true;
}
} 
