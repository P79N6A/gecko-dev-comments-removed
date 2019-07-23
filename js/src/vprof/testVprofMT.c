





































#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "vprof.h"

static void cProbe (void* vprofID)
{
    if (_VAL == _IVAR1) _I64VAR1 ++;
    _IVAR1 = _IVAR0;

    if (_VAL == _IVAR0) _I64VAR0 ++;
    _IVAR0 = (int) _VAL;

    _DVAR0 = ((double)_I64VAR0) / _COUNT;
    _DVAR1 = ((double)_I64VAR1) / _COUNT;
}





#define THREADS 1
#define COUNT 100000
#define SLEEPTIME 0

static int64_t evens = 0;
static int64_t odds = 0;

void sub(int val)
{
    int i;
    
    for (i = 0; i < COUNT; i++) {
        
        
        _vprof (i);
        
        
        
        
        
        
        if (i % 2 == 0) {
            
            
            
            evens ++;
        } else {
            
            _vprof (i, cProbe);
            odds ++;
        }
        
    }
    
}

HANDLE array[THREADS];

static int run (void)
{
    int i;
    
    time_t start_time = time(0);
    
    for (i = 0; i < THREADS; i++) {
        array[i] = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)sub, (LPVOID)i, 0, 0);
    }
    
    for (i = 0; i < THREADS; i++) {
        WaitForSingleObject(array[i], INFINITE);
    }

    return 0;
}

int main ()
{
    DWORD start, end;

    start = GetTickCount ();
    run ();
    end = GetTickCount ();

    printf ("\nRun took %d msecs\n\n", end-start);
}
