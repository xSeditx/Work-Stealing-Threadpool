#include"Threadpool.h"

 
#pragma warning( push )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4018 ) // Optimization off warning of mine

thread_local uint32_t LocalThreadID{ 0 };
uint32_t ThreadCounter{ 0 };
jmp_buf ContextArray[16];

_static std::atomic<int> Threadpool::RunningThreads{ 0 };
std::mutex PrintMtx;

/* ============================================================
 *                    Initializer
 * ============================================================ */
Threadpool::Threadpool()
{// Create a set number of Threads and Add Job Queues to the Threadpool 

    for (int N{ 0 }; N < ThreadCount; ++N)
    {// Creates and Runs Schedular

        Worker_Threads.emplace_back([&, N]
        {
            ThreadQueue[N].QueueID = std::this_thread::get_id();
            Run(N);
        });
    }
}

/* ============================================================ */

int DEBUGVALUE{ 0 };

uint32_t *DebugStack = nullptr;
char     *DebugStackchar = nullptr;
uint64_t *DebugStack64bit = nullptr;

/* ============================================================
 *                    Executors
 * ============================================================ */
void Threadpool::Run(unsigned int _i)
{ // Initializes Thread and starts the Queue running 
	LocalThreadID = ThreadCounter++;

    while (true)
    {// Constantly run until application or user shuts it down

        Executor* Function{ nullptr };

        if (!setjmp(ContextArray[LocalThreadID]))
        {
            for (unsigned int N{ 0 }; N != ThreadCount; ++N)
            {// Cycle over all available Queues until one returns some work 

                if (ThreadQueue[static_cast<size_t>((_i + N) % ThreadCount)].try_Pop(Function))
                {// If Queue N succeeded at returning a function break the for loop and run the function
                    break;
                }
            }

            if (!Function && !ThreadQueue[_i].pop(Function))
            {// If there is no Function and the Queue fails to Pop it means that it is quiting time
                break;
            }

            Print("Invoking function from Run " << typeid(*Function).name());
            Function->Invoke();  // Invoke the returned function
            Print("Returned from Runs Invoke" << typeid(*Function).name());
            delete &(*Function); // Destroy the Object which our Async Class Allocated
        }
        else {
            Print("Returned into the Run Function -----------------------------------");
        }
    }

    Alive = false;
}


/* ============================================================
 *                    Queue Management
 * ============================================================ */


/* Pop Front of the Queue */
bool Threadpool::JobQueue::try_Pop(Executor*& _func)
{// Try to pop a function off the Queue if it fails return false

    /* ~   CRITICAL SECTION   ~ */
    std::unique_lock<std::mutex> Lock{ QueueMutex, std::try_to_lock };
    if (!Lock || TaskQueue.empty())
    {
        return false;
    }

    _func = std::move(TaskQueue.front());
    TaskQueue.pop_front();
    return true;
}

 /*  Pop function from Queue if previous Try pops failed wait on it
 /*  Entire Scope is protected by the Queue Mutex */
bool Threadpool::JobQueue::pop(Executor*& _func)
{

      /* ~   CRITICAL SECTION   ~ */
    std::unique_lock<std::mutex> Lock{ QueueMutex };
    while (TaskQueue.empty() && !is_Done)
    {// If Queue is Empty and we are not Done Wait until there is work to do
        is_Ready.wait(Lock);
    }
    if (TaskQueue.empty())
    {// If Task Queue is empty and we are done, return false to Initiate shut down process
        return false;
    }
    // Move the pointer to the function pointer from our Queue into our _func object

    _func = std::move(TaskQueue.front());
    TaskQueue.pop_front();
    return true;
}

/* Pop Back of the Queue
NOTE: Likely will not need this as much but will need the Push Front
So included this for completeness. Functions with Low Priority could
very well be used here I guess. */
bool Threadpool::JobQueue::try_Pop_back(Executor*& _func)
{// Try to pop a function off the Queue if it fails return false

    /* ~   CRITICAL SECTION   ~ */
    std::unique_lock<std::mutex> Lock{ QueueMutex, std::try_to_lock };
    if (!Lock || TaskQueue.empty())
    {
        return false;
    }

    _func = std::move(TaskQueue.back()); // Gather from the back of the queue this time
    TaskQueue.pop_back(); // Remove it from the Queue
    return true;
}

bool Threadpool::JobQueue::pop_back(Executor*& _func)
{ /*  Pop function from Queue if previous Try pops failed wait on it
      Entire Scope is protected by the Queue Mutex */

      /* ~   CRITICAL SECTION   ~ */
    std::unique_lock<std::mutex> Lock{ QueueMutex };
    while (TaskQueue.empty() && !is_Done)
    {// If Queue is Empty and we are not Done Wait until there is work to do
        is_Ready.wait(Lock);
    }
    if (TaskQueue.empty())
    {// If Task Queue is empty and we are done, return false to Initiate shut down process
        return false;
    }
    // Move the pointer to the function pointer from our Queue into our _func object

    _func = std::move(TaskQueue.back());
    TaskQueue.pop_back();
    return true;
}

/* ============================================================ */




/* ============================================================
 *                    Submitters
 * ============================================================ */
 /* Push onto the Back of the Queue*/
bool Threadpool::JobQueue::try_push(Executor* _func)
{// Attempts to add a function to the Queue if unable to lock return false 

    {/* ~   CRITICAL SECTION   ~ */
        std::unique_lock<std::mutex> Lock{ QueueMutex, std::try_to_lock };
        if (!Lock)
        {// If our mutex is already locked simply return 
            return false;
        }
        TaskQueue.push_back(std::move(_func));    // Else place on the back of our Queue

    }/* ~ END CRITICAL SECTION ~ */              // Unlock the Mutex 

    is_Ready.notify_one();                       // Tell the world about it 
    return true;                                 // Lets Async know you succeeded
}
void Threadpool::JobQueue::push(Executor* _func)
{// Adds a Function to our Queue

    {/* ~   CRITICAL SECTION   ~ */
        std::unique_lock<std::mutex> Lock{ QueueMutex };
        TaskQueue.emplace_back(std::move(_func));

    }/* ~ END CRITICAL SECTION ~ */

    is_Ready.notify_one();                       // Why did I not have this before?
}
/* Push onto the Front of the Queue
NOTE: Needed for higher priority calls such as forking the main
thread and directly calling the child function. A Suspend Ops.*/
bool Threadpool::JobQueue::try_push_front(Executor* _func)
{// Attempts to add a function to the Queue if unable to lock return false 

    {/* ~   CRITICAL SECTION   ~ */

        std::unique_lock<std::mutex> Lock{ QueueMutex, std::try_to_lock };
        if (!Lock)
        {// If our mutex is already locked simply return 
            return false;
        }
        // Push... Emplaced_front instead????
        TaskQueue.push_front(std::move(_func));  // Else place on the back of our Queue

    }/* ~ END CRITICAL SECTION ~ */              // Unlock the Mutex 

    is_Ready.notify_one();                       // Tell the world about it 
    return true;                                 // Lets Async know you succeeded
}
void Threadpool::JobQueue::push_front(Executor* _func)
{// Adds a Function to our Queue

    {/* ~   CRITICAL SECTION   ~ */

        std::unique_lock<std::mutex> Lock{ QueueMutex };
        TaskQueue.emplace_front(std::move(_func));

    }/* ~ END CRITICAL SECTION ~ */

    is_Ready.notify_one();                       // Why did I not have this before?
}

/* ============================================================ */




/* ============================================================
 *                    Destructors
 * ============================================================ */
void Threadpool::JobQueue::Done()
{// Triggers the Threadpool to shut down when the application ends or user ask it to

    {/* ~   CRITICAL SECTION   ~ */

        std::unique_lock<std::mutex> Lock{ QueueMutex };
        is_Done = true;

    }/* ~ END CRITICAL SECTION ~ */

    is_Ready.notify_all();
}
Threadpool::~Threadpool()
{
    for (auto& Q : ThreadQueue)
    {// Signal to all Queues that we are closing down Shop
        Q.Done();
    }
    for (auto& WT : Worker_Threads)
    {// Once those threads Finish we Join all our threads and close the Threadpool
        WT.join();
    }
}

/* ============================================================ */









void _cdecl Enter_frame(StackFrame *sf)
{
    Enter_frame_internal(sf, 0);
}

void _cdecl Enter_frame_1(StackFrame *sf)
{
    Enter_frame_internal(sf, 1);
    sf->Reserved = 0;
}
void _cdecl Enter_frame_fast(StackFrame *sf)
{
    Enter_frame_fast_internal(sf, 0);
}

void _cdecl  Enter_frame_fast_1(StackFrame *sf)
{
    Enter_frame_fast_internal(sf, 1);
    sf->Reserved = 0;
}
#include<Windows.h>
void* Get_current_thread_id(void)
{
    return (void*)(size_t)GetCurrentThreadId();
}

int  Get_Hardware_CPU_Count(void)
{
    static int active_processors = 0;
    SYSTEM_INFO info;

    // If we've already done this, just return the value we calculated last
    // time.  It's not going to change...
    if (active_processors > 0)
        return active_processors;

    // If we've got a function to count up all the processors across all the
    // processor groups, use it.  It will return 0 if it fails.
    //  win_init_processor_groups();
    //  if (NULL != s_pfnGetActiveProcessorCount)
    //  {
    //  	active_processors =
    //  		s_pfnGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    //  	if (active_processors > 0)
    //  		return active_processors;
    //  }

    // Use the old function to ask Windows for the number of processors in the
    // system.  If this is an older OS, then there's no concept of processor
    // groups, and this is the total number of processors.  If this OS supports
    // >64 processors, and GetActiveProcessorCount returned an error, returning
    // the number of processors in the group this thread is executing on is the
    // best we can do
    info.dwNumberOfProcessors = 0;
    GetSystemInfo(&info);

    active_processors = info.dwNumberOfProcessors;

    return active_processors;
}








#pragma warning( pop )



 /*
==========================================================================================================================================================================
														   TRASH:
==========================================================================================================================================================================
*/
/*



	template<typename _Ty>
	struct ContinuationFuture
		:
		public std::future<_Ty>
	{
		_Ty get()
		{
			Print("Continuation Future Get");
		}
	};

	template<typename _Ty>
	struct ContinuationPromise
		:
		public std::promise<_Ty>
	{
		auto get_future()
		{
			Print("Continuation Promise Get Future"); return std::move(RestorePoint);
		}

//_NODISCARD future<_Ty> get_future()
//{	// return a future object that shares the associated
//	// asynchronous state
//	return (future<_Ty>(_MyPromise._Get_state_for_future(), _Nil()));
//}
//
//void set_value(const _Ty& _Val)
//{	// store result
//	_MyPromise._Get_state_for_set()._Set_value(_Val, false);
//}

		ContinuationFuture<_Ty> RestorePoint;
	};







		/* gshandlereh.cpp
		__CxxFrameHandler3(
							ExceptionRecord,
							EstablisherFrame,
							ContextRecord,
							DispatcherContext
							);




			//if (setjmp(*((jmp_buf*)&Buffer)) != 1)//Context
			//{
			//	memcpy(&Context, &Buffer, sizeof(jmp_buf));
			//	ReturnValue.set_value(Context);
			//}
			//else
			//{
			//	Print("Returning from the Jump " << DEBUGSTORE);
			//}




				Print("Created Context");

				Print("Returned from Child Function Call");

				if (Stolen == true)
				{
					Print("Returned from Child After being stolen");
				}
				else
				{
					Print("Returned from Child Before being stolen");
				}
				Print("Returning From Jump Call");
				if (FinishedChild == true)
				{
					Print("Calling after child function had returned");
				}
				else
				{
					Print("Calling before child function had returned");
				}

		__forceinline void CreateSuspendPoint()
		{
			if (setjmp(Context) != 1)
			{
				Print("Suspend Context Set");
			}
			else
			{
				Print("Suspend Context Returned");
			}
		}



//void InvokeChildProcess()
//{
//	Print("Invoking the Child now");
//	ReturnValue.set_value(std::apply(Function, Arguments));
//	Print("Returning from child now");
//	FinishedChild = true;
//}

// ContextArray[LocalThreadID]->Part[0];
			//	Context[0] = B2.Esp;// ContextArray[LocalThreadID]->Part[0];
			//	Context->
				
				//Buffer.Rsp = C2.

uint64_t Frame = Context.[0].Part[0];
uint64_t SP =    Context.[1].Part[0];
uint64_t BP =    Context.[1].Part[1];
uint64_t DI =    Context.[2].Part[1];
uint64_t IP =    Context.[5].Part[0];
uint64_t MxCsr = Context.[5].Part[1];
*/

//Frame = (void*)_function->Context[0].Part[0];
//SP = _function->Context[1].Part[0];
//BP = _function->Context[1].Part[1];
//DI = _function->Context[2].Part[1];
//IP = _function->Context[5].Part[0];
//MxCsr = _function->Context[5].Part[1];
