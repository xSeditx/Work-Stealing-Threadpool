#pragma once
#ifndef THREADPOOL_H
#define THREADPOOL_H

/*=======================================================================
                     # ThreadPool Module  #
     Handles the Acceptance and dispatching of Asynchronous Function
  calls via Threaded Queues which store pointers to functions and
  systematically keeps every core of the CPU busy at all times. As soon
  as one function returns another is popped from a stack and ran and the
  user synchronizes these efforts via a future object which becomes valid
  with the return value after the desired function has been properly
  run.
 =======================================================================*/


 /*************************************************************************/
 /*                       This file is part of:                           */
 /*                       Creature Game Engine                            */
 /*              https://github.com/xSeditx/Creature-Engine               */
 /*************************************************************************/
 /* Copyright (c) 2019 Sedit                                              */
 /*                                                                       */
 /* Permission is hereby granted, free of charge, to any person obtaining */
 /* a copy of this software and associated documentation files (the       */
 /* "Software"), to deal in the Software without restriction, including   */
 /* without limitation the rights to use, copy, modify, merge, publish,   */
 /* distribute, sublicense, and/or sell copies of the Software, and to    */
 /* permit persons to whom the Software is furnished to do so, subject to */
 /* the following conditions:                                             */
 /*                                                                       */
 /* The above copyright notice and this permission notice shall be        */
 /* included in all copies or substantial portions of the Software.       */
 /*                                                                       */
 /* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
 /* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
 /* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
 /* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
 /* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
 /* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
 /* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
 /*=======================================================================*/


#include <thread>
#include <future>
//#include"Experimental/Future.h"
#include <deque>
#include <tuple>
#include <iostream>
#include <type_traits> 
#include <setjmp.h>

/* Ensure we have access to C++ 17 functionality */
#if !_HAS_CXX17
#    error " C++ 17 functionality needed Please enable C++ Language Standard /std::c++17"
#endif

#pragma warning( push )
#pragma warning( disable : 4244 ) // Type conversions
#pragma warning( disable : 4018 ) // Optimization off warning of mine

/*====================================================================================================================================*/
/*           Normally these Macros and defines would be placed in a Common header somewhere but this is a self contained module       */
/*====================================================================================================================================*/
typedef void(*Fptr)();

/* Denotes that Object Can not be Copied or Assigned */
#ifndef NO_COPY_OR_ASSIGNMENT
#    define NO_COPY_OR_ASSIGNMENT(Class_X)	void operator=(const Class_X&) = delete;\
Class_X(const Class_X&) = delete
#endif
#ifndef NO_VTABLE
#    define NO_VTABLE __declspec(novtable) 
#endif

/* Just to do basic debug logging to the console protected with a Mutex to avoid Slicing of the Text*/
extern std::mutex PrintMtx;//PrintMtx.lock();
#ifndef Print
#    define Print(x)  std::cout << std::this_thread::get_id() << " ";\
std::cout << x << " \n";\
std::cout << "      " << typeid(*this).name()\
<< "\n\n";

//PrintMtx.unlock();

#endif
#define _static // Just used to aid in code understanding

#define QueueDebug(x) Print("Queue ID: " << DEBUGSTORE); Print("ThreadID = " << std::this_thread::get_id())

#define makeAddress(_val)    (void*)*&(_val)
#define GET_SP(ctx) ((_JUMP_BUFFER*)(&(ctx)))->Rsp
#define GET_FP(ctx) ((_JUMP_BUFFER*)(&(ctx)))->Rbp
#define GET_PC(ctx) ((_JUMP_BUFFER*)(&(ctx)))->Rip
#define FP(SF) GET_FP((SF)->Context)
#define PC(SF) GET_PC((SF)->Context)
#define SP(SF) GET_SP((SF)->Context)


#define GET_REG(buffer) ((_JUMP_BUFFER *)&buffer)


inline size_t Get_frame_size()
{
    jmp_buf Context;
    return (size_t)GET_FP(Context) - (size_t)GET_SP(Context);
}
//======================================================================================
/* ==================================================================================================================================== */

extern uint32_t *DebugStack;
extern char *DebugStackchar;
extern uint64_t *DebugStack64bit;


/*      Non-blocking test of std::future to see if value is avalible yet
/*	NOTE: Performance of this is not the best use sparingly our outside of hot loops */
template<typename _R>
bool is_ready(std::future<_R> const& _fut)
{
    return _fut.valid() ? _fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready : false;
}


extern int DEBUGVALUE;


extern thread_local uint32_t LocalThreadID;
extern uint32_t ThreadCounter;
extern jmp_buf ContextArray[16];

template<typename _Ty>
struct continuationFuture
	: std::future<_Ty> 
{
	_Ty get() 
	{
		longjmp(Context, 1);
		return get;
	}

	void set_context(jmp_buf &_context)
	{
		Context = _context;
	}
	jmp_buf Context;
};

template<typename _Ty>
struct continuationPromise
	: public std::promise<_Ty> 
{
	void set_context(jmp_buf &_context)
	{
		RestorePoint.set_context(_context);
	}
	auto get_future()
	{
		return RestorePoint;
	}

	continuationFuture<_Ty> RestorePoint;
};


#define CILK_FRAME_STOLEN    0x01
#define CILK_FRAME_UNSYNCHED 0x02
#define CILK_FRAME_DETACHED  0x04
#define CILK_FRAME_EXCEPTING 0x10
#define CILK_FRAME_LAST	     0x80
#define CILK_FRAME_EXITING   0x0100
#define CILK_FRAME_UNWINDING 0x10000
#define CILK_FRAME_SUSPENDED 0x8000
#define CILK_FRAME_EXCEPTION_PROBED 0x08
#define CILK_FRAME_SF_PEDIGREE_UNSYNCHED 0x20

enum FrameStatus
{
	Stolen = 0x01,
	Unsynched = 0x02,
	Detached = 0x04,
	Excepting = 0x10,
	Last = 0x80,
	Exiting = 0x0100,
	Unwinding = 0x10000,
	Suspended = 0x8000,
};

struct StackFrame
{
	__forceinline StackFrame()
	{
		EFLAGS = __getcallerseflags();
		AddressOf = _AddressOfReturnAddress();
		ReturnAddress = _ReturnAddress();
		if (setjmp(Context) == 0)
		{
			// Backup State
		}
		else 
		{
			//Restore State 
		}
	}
	[[noreturn]]__forceinline  void Restore()
	{
		longjmp(Context, 1);
	}
	uint32_t    Flags{ 0 };
	FrameStatus Status;   // Will replace the Above Flags
	size_t Size{ 0 }; // Size of Stack Frame. Windows defaults 1024kb or 1mb

	jmp_buf Context{}; //__CILK_JUMP_BUFFER ctx;

	uint32_t mxcsr; /* These are stored in 64 bit context but keep here for now */
	uint16_t fpcsr; /* ======================================================== */

	/**  PARENT
	 * call_parent points to the __cilkrts_stack_frame of the closest
	 * ancestor spawning function, including spawn helpers, of this frame.
	 * It forms a linked list ending at the first stolen frame.
	 */
	void *Parent{ nullptr };
	/**   WORKER 
	 * The client copies the worker from TLS here when initializing
	 * the structure.  The runtime ensures that the field always points
	 * to the __cilkrts_worker which currently "owns" the frame.
	 */
	void* Worker{ nullptr };
	void *except_data{ nullptr };// Maybe...
	void *ReturnAddress{nullptr};// ParentFunction
	void *AddressOf{nullptr};// Maybe Parent idk...
	void* Reserved{ nullptr };


	unsigned int EFLAGS{ 0 }; // Callers Flags
};
 

class Threadpool
{
    NO_COPY_OR_ASSIGNMENT(Threadpool);

    std::thread::id Main_ThreadID{ std::this_thread::get_id() }; // Thread ID of the Main Thread
    static std::atomic<int> RunningThreads;

    /*      WRAPPER_BASE: Allows us to make a polymorphic object and derive from it with the various
    /*	Function types the user may invoke. We store the Base class pointer in Queues to Erase the type
    /*	While Polymorphically calling each Functions specific invoke method */
    struct NO_VTABLE Executor
    {
        virtual ~Executor() noexcept = default;

        /* Function responsible to properly invoking our derived class */
        virtual void Invoke() noexcept = 0;

        /* Mainly for Debug information Gives the Current status of a Function passed into our Queue */
        enum asyncStatus
        {
            Empty, Valid, Waiting, Busy, Submitted, Ready, Aquired
        } Status{ Empty };

        std::thread::id LaunchThread{ std::this_thread::get_id() };
    }; // End Wrapper_Base Class

    /*      ASYNC TASK: Object Binds Function Pointers as well as Arguments into a single unit
        and stores its return value inside of an std::promise<_Rty> With _Rty being functions return type */
    template<typename _Func, typename ...ARGS>
    struct asyncTask final
        : public Executor
    {
        NO_COPY_OR_ASSIGNMENT(asyncTask);

    public:
        using type = std::invoke_result_t<_Func, ARGS...>; // Trying this to avoid C++ 17 Return type of our function

        virtual ~asyncTask() noexcept = default; // Virtual destructor to ensure proper Deallocation of object

        /* Accepts Functions and their arguments */
        asyncTask(_Func&& _function, ARGS&&... _args) noexcept
            :
            Function(std::forward<_Func>(_function)),
            Arguments(std::forward<ARGS>(_args)...)
        {// Signals to user the object is now completed and valid
            Status = Valid;
        }
        

        /*     Calls the Objects Stored function along with its parameters using std::apply
        /*	Sets the value of the Promise and signals to the User that the value is waiting */
        virtual void Invoke() noexcept override
        {
            Status = Busy;
            auto result = std::apply(Function, Arguments);
            ReturnValue.set_value(result);
            Status = Waiting;
        }
 
        /*      To ensure familiarity and usability get_future works to retrieve the
            std::future object associated with the return values std::promise */
        auto get_future() noexcept
        {
            Status = Submitted;
            return ReturnValue.get_future();
        }

    private:
        using Fptr = type(*)(ARGS...);                             // Function pointer type for our function
        const Fptr Function;                                       // Pointer to our Function
        const std::tuple<ARGS...> Arguments;                       // Tuple which Binds the Parameters to the Function call				
#ifdef _EXPERIMENTAL
		Promise<type> ReturnValue;
#else
		std::promise<type> ReturnValue;                            // Return Value of our function stored as a Promise
#endif
    };// End asyncTask Class


	template<typename _Func, typename ...ARGS>
	struct suspendPoint final
		: public Executor
	{
		NO_COPY_OR_ASSIGNMENT(suspendPoint);
		int DEBUGSTORE{ 0 };

	public:
		using type = std::invoke_result_t<_Func, ARGS...>; // Trying this to avoid C++ 17 Return type of our function

		virtual ~suspendPoint() noexcept = default; // Virtual destructor to ensure proper Deallocation of object

		/* Accepts Functions and their arguments */
		suspendPoint(_Func&& _function, ARGS&&... _args) noexcept
			:
			Function(std::forward<_Func>(_function)),
			Arguments(std::forward<ARGS> (_args)...)
		{ 
			DEBUGSTORE = ++DEBUGVALUE;
			Print("Suspend Point ctor " << DEBUGSTORE);
			memset(&Context, 0, sizeof(jmp_buf));
			Status = Valid;
		}

        uint64_t FramePtr{ 0 };
        uint64_t StackPtr{ 0 };
        uint64_t BasePtr{ 0 };
        uint64_t ProgCPtr{ 0 };

        size_t StackSize{ 0 };
        uint32_t *Stack{ nullptr };

        std::atomic<bool> FinishedChild{ false };
        std::atomic<bool> Stolen{ false };


		auto get_future() noexcept
		{


            if (setjmp(Context) == 0)
            {// First pass we execute Child and store context

            int A = 666;
            int B = 555;
            int C = 444;

                FramePtr = Context[0].Part[0];
                StackPtr = Context[1].Part[0];
                BasePtr = Context[1].Part[1];
                StackSize = BasePtr - FramePtr;
                Stack = (uint32_t*)(new char[StackSize* sizeof(int)]);
                memcpy(Stack, makeAddress(FramePtr), StackSize );
 

                Print("Invoking the Child now");
                {
                    //---------- EXECUTE CHILD FUNCTION ---------------------
                    ReturnValue.set_value(std::apply(Function, Arguments));
                    FinishedChild.store(true);
                    //-------------------------------------------------------
                }
                Print("Returning from child now");

                if (Stolen == true)
                {// Means Original path no longer exist and is being run in seperate thread. 
                    Print("Appears to be stole to the Child process so we jump back to Run");
                    longjmp(ContextArray[LocalThreadID], 1);
                }/// ^^^ If we continue elsewhere \/\/\/ If we continue here.
                Print("Does not appear to be stolen so we return back to the Calling thread");
                Status = Submitted;
                longjmp(Context, 1);
                /* Returns the Future of the Child Function.
                /* We may need a Sync in the Get Function for Future */
            }
            else
            {// We return and continue execution here possibly from another thread
                Print("Returning to Thread of Execution where Child was Launched");
                return ReturnValue.get_future();
            }
            Print("~~~~~~~~~~~~~~~~~~~~~~We should never get here ~~~~~~~~~~~~~~~~")
        }

		[[noreturn]] virtual void Invoke() noexcept override
		{
			Status = Busy;
			Print("Invoking the Continuation");

			if (FinishedChild == true)
			{// Child completed and returns to thread of execution.
                /* Child should be on Original path now... We jump back to Threadpool. 
                /* NOTE: Should we??? */
				Print("Invoke after child function had returned");
				longjmp(ContextArray[LocalThreadID], 1);
				return;
			}
			else
			{// Child is in the way of Original thread... We continue on where it left off here. 
                Stolen.store(true);
				Print("Invoke before child function had returned");
 			  jmp_buf C2;
			 	setjmp(C2);		
                GET_REG(Context)->Frame = GET_REG(C2)->Frame;
                GET_REG(Context)->Rsp = GET_REG(C2)->Rsp;
                GET_REG(Context)->Rbp = GET_REG(C2)->Rbp;
               // Context[0].Part[0] = C2[0].Part[0];
               // Context[1].Part[0] = C2[1].Part[0];
               // Context[1].Part[1] = C2[1].Part[1];
                //	_JUMP_BUFFER B2 = *(_JUMP_BUFFER*)&C2;
			 //	Context->Part[0] = B2.Rsp;
				longjmp(Context, 1);

                Print("~~~~~~~~~~~~~~~~ NEVER REACH ~~~~~~~~~~~~~~~~~~")
			}
			Status = Waiting;		    
  		}

		jmp_buf Context;
		std::promise<type> ReturnValue;
		_JUMP_BUFFER Buffer{};

	private:
		using Fptr = type(*)(ARGS...);                             // Function pointer type for our function
		const Fptr Function;                                       // Pointer to our Function
		const std::tuple<ARGS...> Arguments;                       // Tuple which Binds the Parameters to the Function call				
#ifdef _EXPERIMENTAL
		Promise<type> ReturnValue;
#else
		//ContinuationPromise<type> ReturnValue;                            // Return Value of our function stored as a Promise
	   // continuationPromise<type> ReturnValue;
#endif
		//https://www.ibm.com/support/knowledgecenter/SSLTBW_2.1.0/com.ibm.zos.v2r1.bpxbd00/r0stjm.htm
	};

public:

    /*
    /* JOB QUEUE: Stores a Deque of Base Pointers to asyncTask Objects.
    /* Pointers are pushed and popped off the stack and properly deallocated after no longer needed */
    struct JobQueue
    {
    public:
        std::thread::id QueueID{ std::this_thread::get_id() };

        JobQueue() = default;

        std::mutex QueueMutex;
        std::deque<Executor*> TaskQueue;
        std::condition_variable is_Ready;
        bool is_Done{ false };

        /* Triggers the Threadpool to shut down when the application ends or user ask it to */
        void Done();

        /* Try to Pop a function off the Queue if it fails return false */
        bool try_Pop(Executor*& _func);

        /* Pop function from Queue if fails wait for it */
        bool pop(Executor*& _func);

        /* Attempts to add a function to the Queue if unable to lock return false */
        bool try_push(Executor* _func);

        /* Adds a Function to our Queue */
        void push(Executor* _func);



        /* Try to Pop a function off the BACK OF the Queue if it fails return false */
        bool try_Pop_back(Executor*& _func);            // Lower Priority Function

        /* Pop function from Queue if fails wait for it */
        bool pop_back(Executor*& _func);                // Lower Priority Function

        /* Attempts to add a function to the FRONT OF the Queue if unable to lock return false */
        bool try_push_front(Executor* _func);           // Higher Priority Function

        /* Adds a Function to the FRONT OF our Queue */
        void push_front(Executor* _func);               // Higher Priority Function
    };


    const uint32_t           ThreadCount{ std::thread::hardware_concurrency() };
    std::vector<JobQueue>    ThreadQueue{ ThreadCount };
    std::vector<std::thread> Worker_Threads; // Each Thread is responsible for an instance of Run which owns a specific Queue
    std::atomic<uint32_t>    Index{ 0 };
    
    /* Manages the state of the Thread pool */
    bool Alive       { true };
    bool is_Alive()  { return Alive; }
    void Terminate() { Alive = false; }

    /*
    /* Create a set number of Threads and Add Job Queues to the Threadpool
    /*     NOTE: May Possibly add a Number here to create a specific number of threads */
    Threadpool();

    /* Properly shuts down our Threadpool and joins any Threads running */
    ~Threadpool();

    /* Initializes Thread and starts the Queue running */
    void Run(unsigned int _i);

    /*     Since C++11, initialization of function scope static variables is thread safe :
    /* the first tread calling get() will initialize instance,
    /* blocking other threads until the initialization is completed. All subsequent calls will use the initialized value.
    /* Source: https://stackoverflow.com/questions/27181645/is-publishing-of-magic-statics-thread-safe 				
    /* 
    /* Returns a singleton instance of our Threadpool */
    static Threadpool& get()
    {
        static Threadpool __instance;
        return __instance;
    }


    /* Executor for our Threadpool Allocating our Asyncronous objects,
    /* returning their Futures an handles work sharing throughout all the available Queues */
#ifdef _EXPERIMENTAL
	template<typename _FUNC, typename...ARGS >
	auto Async(_FUNC&& _func, ARGS&&... args)->Future<typename asyncTask<_FUNC, ARGS... >::type>
	{// Accept arbitrary Function signature, Bind its arguments and add to a Work pool for Asynchronous execution
		auto i = Index++;// Increases the first thread we test by one each call ensuring better work distribution
		int Attempts = 5;// Ensure fair work distribution

		if(std::this_thread::get_id() != Main_ThreadID)
		{
			auto _function = new suspendPoint<_FUNC, ARGS... >(std::move(_func), std::forward<ARGS>(args)...);  // Create our task which binds the functions parameters
			auto result = _function->get_future(); // Get the future of our async task for later use

			for (unsigned int n{ 0 }; n != ThreadCount * Attempts; ++n) // Attempts is Tunable for better work distribution
			{// Cycle over all Queues K times and attempt to push our function to one of them

				if (ThreadQueue[static_cast<size_t>((i + n) % ThreadCount)].try_push_front(static_cast<Executor*>(_function)))
				{// If succeeded return our functions Future
					return result;
				}
			}

			ThreadQueue[i % ThreadCount].push_front(static_cast<Executor*>(_function));
			return result;
		}
		else
		{
			auto _function = new asyncTask<_FUNC, ARGS... >(std::move(_func), std::forward<ARGS>(args)...);  // Create our task which binds the functions parameters
			auto result = _function->get_future(); // Get the future of our async task for later use

			for (unsigned int n{ 0 }; n != ThreadCount * Attempts; ++n) // Attempts is Tunable for better work distribution
			{// Cycle over all Queues K times and attempt to push our function to one of them

				if (ThreadQueue[static_cast<size_t>((i + n) % ThreadCount)].try_push(static_cast<Executor*>(_function)))
				{// If succeeded return our functions Future
					return result;
				}
			}

			// In the rare instance that all attempts at adding work fail just push it to the Owned Queue for this thread
			ThreadQueue[i % ThreadCount].push(static_cast<Executor*>(_function));
			return result;
		}
	}
#else
	template<typename _FUNC, typename...ARGS >
	auto Async(_FUNC&& _func, ARGS&&... args)->std::future<typename asyncTask<_FUNC, ARGS... >::type>
	{// Accept arbitrary Function signature, Bind its arguments and add to a Work pool for Asynchronous execution
        int  TEST_STACK = 666;
        int  Test2 = 555;
        int  Test3= 444;
        int  Test4 = 333;
        char carTest0 = 000;
        char carTest = 222;
        char carTest2 = 111;
        char carTest3 = 123;

        auto i = Index++;// Increases the first thread we test by one each call ensuring better work distribution
		int Attempts = 5;// Ensure fair work distribution

		if (std::this_thread::get_id() != Main_ThreadID)
		{
           void *Frame = nullptr;
           uint64_t SP =   	0;	//int *Return = (int*)_ReturnAddress();
           uint64_t BP =   	0;	//void*returnAddof =  _AddressOfReturnAddress();
           uint64_t DI =    0;
           uint64_t IP =   	0;	
           uint64_t MxCsr =	0;
           size_t StackSize = 1300 ;// Frame - SP;

           Print("Creating Suspend in Async"); 	
           /*
           Get this context.
             Frame = [0]->Part[0]
             SP = [1]->Part[0]
             BP = [1]->Part[1]
            */
           auto _function = new suspendPoint<_FUNC, ARGS... >(std::move(_func), std::forward<ARGS>(args)...);  // Create our task which binds the functions parameters
			//auto result = _function->get_future(); // Get the future of our async task for later use
			if (!setjmp(_function->Context))// == 0) 
			{// 
                Frame = (void*) _function->Context[0].Part[0];
                SP =    _function->Context[1].Part[0];
                BP =    _function->Context[1].Part[1];
                DI =    _function->Context[2].Part[1];
                IP =    _function->Context[5].Part[0];
                MxCsr = _function->Context[5].Part[1];
                DebugStack = new uint32_t[StackSize];
              
                DebugStackchar = (char*)DebugStack;
                DebugStack64bit = (uint64_t*)DebugStack;

                memcpy(DebugStack, Frame, StackSize);
                std::cout << "Stacks: " << DebugStackchar << DebugStack64bit << "\n";
				_function->Buffer = *(_JUMP_BUFFER*)&_function->Context;

                std::cout << TEST_STACK <<
                    Test2 <<
                    Test3 <<
                    Test4 <<
                    carTest0 <<
                    carTest <<
                    carTest2 <<
                    carTest3 << "END: \n";



			//	Print("Set the context here for first time....");
				for (unsigned int n{ 0 }; n != ThreadCount * Attempts; ++n) // Attempts is Tunable for better work distribution
				{// Cycle over all Queues K times and attempt to push our function to one of them

					if (ThreadQueue[static_cast<size_t>((i + n) % ThreadCount)].try_push_front(static_cast<Executor*>(_function)))
					{// If succeeded return our functions Future
						Print("We have pushed the continuation to the Threadpool and are about to run the child");
						Print("Suspend returns here A");
						auto result = _function->get_future();// result;
						Print("Really Returning A");
						return result;
					}
				}

				ThreadQueue[i % ThreadCount].push_front(static_cast<Executor*>(_function));
				Print("We have pushed the continuation to the Threadpool and are about to run the child");
				Print("Suspend returns here B");
				auto result = _function->get_future();// result;
				Print("Really Returning B");
				return result;
			}
            else
            {// Comes here if the Task has been stolen before the Child continues
                jmp_buf Temp;
                if(!setjmp(Temp))
                { 
                    //Temp[0].Part[0] = 
                    //Temp[].Part[] =
                    Frame = (void*)_function->Context[0].Part[0];
                    SP = _function->Context[1].Part[0];
                    BP = _function->Context[1].Part[1];
                    DI = _function->Context[2].Part[1];
                    IP = _function->Context[5].Part[0];
                    MxCsr = _function->Context[5].Part[1];

                        //__debugbreak();
                }
                else 
                {
                    __debugbreak(); 
                    return _function->ReturnValue.get_future();
                }
                Print("Parent has beenu Stolen Before Child Returned");
                __debugbreak();
                longjmp(Temp, 1); //Jumps back above with the new register values
            }
		}// END SUSPEND POINT



		else
		{// IF we have called from the main thread and are NOT making a suspend point... 
			Print("Creating Task in Async");
			auto _function = new asyncTask<_FUNC, ARGS... >(std::move(_func), std::forward<ARGS>(args)...);  // Create our task which binds the functions parameters
			auto result = _function->get_future(); // Get the future of our async task for later use
			auto i = Index++;// Increases the first thread we test by one each call ensuring better work distribution

			int Attempts = 5;// Ensure fair work distribution
			for (unsigned int n{ 0 }; n != ThreadCount * Attempts; ++n) // Attempts is Tunable for better work distribution
			{// Cycle over all Queues K times and attempt to push our function to one of them

				if (ThreadQueue[static_cast<size_t>((i + n) % ThreadCount)].try_push(static_cast<Executor*>(_function)))
				{// If succeeded return our functions Future
					Print("Really Returning D");
					return result;
				}
			}

			// In the rare instance that all attempts at adding work fail just push it to the Owned Queue for this thread
			ThreadQueue[i % ThreadCount].push(static_cast<Executor*>(_function));
			Print("Really Returning E");
			return result;
		}


		Print("Do we ever get here?");
    }
#endif
}; // End ThreadPool Class


#pragma warning( pop )
#endif// THREADPOOL_H



 /*
==========================================================================================================================================================================
                                                           NOTES:
==========================================================================================================================================================================
*/



/*
The Implementation of the Cilk-5 Multithreaded Language
http://supertech.csail.mit.edu/papers/cilk5.pdf

Create 
https://stackoverflow.com/questions/26605063/an-invalid-or-unaligned-stack-was-encountered-during-an-unwind-operation?noredirect=1&lq=1


If Launched from Threaded Task... 
Get the Context of that Thread...
Push the Context of that to the Threadpool.
AFTER we have pushed the context to the Threadpool Then and only then should we run the Child

in the Invoke... If the Child already returned... Do nothing and return to delete this Context
Else...
in the Invoke if the Child did NOT return Invoke the continuation which is the Context the suspend point created.

  Main 
    |
	|
  Async
  {
      Push
	  RunChild
  }

*/


/*============================================================
                        CILK STUFF
============================================================



static inline
void enter_frame_internal(__cilkrts_stack_frame *sf, uint32_t version)
{
	__cilkrts_worker *w = __cilkrts_get_tls_worker();
	if (w == 0)
	{ 
        w = BIND_THREAD_RTN();
        
        sf->flags = CILK_FRAME_LAST | (version << 24);
        CILK_ASSERT((sf->flags & CILK_FRAME_FLAGS_MASK) == CILK_FRAME_LAST);
	}
    else 
    {
        sf->flags = (version << 24);
        CILK_ASSERT((sf->flags & CILK_FRAME_FLAGS_MASK) == 0);
	}
	sf->call_parent = w->current_stack_frame;
	sf->worker = w;
	w->current_stack_frame = sf;
}



CILK_API_INT __cilkrts_synched(void)
{
	__cilkrts_worker *w = __cilkrts_get_tls_worker();
 	if (NULL == w)
	{
	    return 1;
	}
 	uint32_t flags = w->current_stack_frame->flags;
	if (0 == (flags & CILK_FRAME_UNSYNCHED))
	{
		return 1;

	}
	full_frame *ff = w->l->frame_ff;
	if (NULL == ff)
	{
		return 1;
	}
 	return 1 == ff->join_counter;
}



The return address
Argument variables passed on the stack
Local variables (in HLLs)
Saved copies of any registers modified by the subprogram that need to be restored (e.g. $s0 - $s8 in MAL).


                jmp_buf Buffer;
                if (setjmp(Buffer) == 0)
                {
                    //Print("Get Stackptr");
                    Buffer[0].Part[0] = SP;
                    Buffer[1].Part[1] = BP;

                //		//_function->Buffer.Rsp;
                }
                else
                {
                    return _function->ReturnValue.get_future();
                }
                //DebugStackchar = (char*)DebugStack;
                void *Addy = makeAddress(Buffer[0].Part[0]);//(void*)*&Buffer[0].Part[0];
                memcpy(Addy ,DebugStack, StackSize);
                Buffer[0].Part[0] = (int)Addy;
                Print("Setting the Stack");
                longjmp(Buffer, 1);
                return _function->ReturnValue.get_future();
            }
            Print("Idk what this would be or if its possible...");

*/



#include<Windows.h>


static __forceinline void Enter_frame_internal(StackFrame *sf, uint32_t version)
{
	//__cilkrts_worker *w = __cilkrts_get_tls_worker();
	if (sf->Worker == nullptr) { /* slow path */
	//	w = BIND_THREAD_RTN();

		sf->Flags = CILK_FRAME_LAST | (version << 24);
		//CILK_ASSERT((sf->flags & CILK_FRAME_FLAGS_MASK) == CILK_FRAME_LAST);
	}
	else {
		sf->Flags = (version << 24);
		//CILK_ASSERT((sf->flags & CILK_FRAME_FLAGS_MASK) == 0);
	}
	//sf->call_parent = w->current_stack_frame;
	//sf->worker = w;
	//w->current_stack_frame = sf;
}
static __forceinline void Enter_frame_fast_internal(StackFrame *sf, uint32_t version)
{
	//__cilkrts_worker *w = __cilkrts_get_tls_worker_fast();
	sf->Flags = version << 24;
	//sf->Parent = w->current_stack_frame;
	//sf->Worker = w;
	//w->current_stack_frame = sf;
}

void _cdecl Enter_frame(StackFrame *sf);
void _cdecl Enter_frame_1(StackFrame *sf);
void _cdecl Enter_frame_fast(StackFrame *sf);
void _cdecl Enter_frame_fast_1(StackFrame *sf);
void* Get_current_thread_id(void);

int  Get_Hardware_CPU_Count(void);




/*
function A:
push space for the return value
push parameters
push the return address
jump to the function B
function B:
push the address of the previous stack frame
push values of registers that this function uses (so they can be restored)
push space for local variables
do the necessary computation
restore the registers
restore the previous stack frame
store the function result
jump to the return address
function A:
pop the parameters
pop the return value

*/