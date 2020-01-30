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
#include"Experimental/Future.h"
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

/* Denotes that Object Can not be Copied or Assigned */
#ifndef NO_COPY_OR_ASSIGNMENT
#    define NO_COPY_OR_ASSIGNMENT(Class_X)	void operator=(const Class_X&) = delete;\
Class_X(const Class_X&) = delete
#endif
#ifndef NO_VTABLE
#    define NO_VTABLE __declspec(novtable) 
#endif

/* Just to do basic debug logging to the console protected with a Mutex to avoid Slicing of the Text*/
extern std::mutex PrintMtx;
#ifndef Print
#    define Print(x)  PrintMtx.lock(); std::cout << x << "\n"; PrintMtx.unlock();
#endif
#define _static // Just used to aid in code understanding
//======================================================================================
/* ==================================================================================================================================== */



/*      Non-blocking test of std::future to see if value is avalible yet
/*	NOTE: Performance of this is not the best use sparingly our outside of hot loops */
template<typename _R>
bool is_ready(std::future<_R> const& _fut)
{
    return _fut.valid() ? _fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready : false;
}




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
		NO_COPY_OR_ASSIGNMENT(asyncTask);

	public:
		using type = std::invoke_result_t<_Func, ARGS...>; // Trying this to avoid C++ 17 Return type of our function

		virtual ~suspendPoint() noexcept = default; // Virtual destructor to ensure proper Deallocation of object

		/* Accepts Functions and their arguments */
		suspendPoint(_Func&& _function, ARGS&&... _args) noexcept
			:
			Function(std::forward<_Func>(_function)),
			Arguments(std::forward<ARGS>(_args)...)
		{// Signals to user the object is now completed and valid
			Status = Valid;
			if (set_jmp(Context) != 1)
			{
				Print("Setting the Jump");

				ReturnValue.set_value(Function(Arguments));
			}
			else {
				Print("Returning from the Jump");
			}
		}


		/*     Calls the Objects Stored function along with its parameters using std::apply
		/*	Sets the value of the Promise and signals to the User that the value is waiting */
		virtual void Invoke() noexcept override
		{
			Status = Busy;
			longjmp(Context, 1);
			//auto result = std::apply(Function, Arguments);
			//ReturnValue.set_value(result);
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
		jmp_buf Context;
		using Fptr = type(*)(ARGS...);                             // Function pointer type for our function
		const Fptr Function;                                       // Pointer to our Function
		const std::tuple<ARGS...> Arguments;                       // Tuple which Binds the Parameters to the Function call				
#ifdef _EXPERIMENTAL
		Promise<type> ReturnValue;
#else
		std::promise<type> ReturnValue;                            // Return Value of our function stored as a Promise
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

		auto _function = new asyncTask<_FUNC, ARGS... >(std::move(_func), std::forward<ARGS>(args)...);  // Create our task which binds the functions parameters
		auto result = _function->get_future(); // Get the future of our async task for later use
		auto i = Index++;// Increases the first thread we test by one each call ensuring better work distribution

		int Attempts = 5;// Ensure fair work distribution
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
#else
    template<typename _FUNC, typename...ARGS >
    auto Async(_FUNC&& _func, ARGS&&... args)->std::future<typename asyncTask<_FUNC, ARGS... >::type>
    {// Accept arbitrary Function signature, Bind its arguments and add to a Work pool for Asynchronous execution

        auto _function = new asyncTask<_FUNC, ARGS... >(std::move(_func), std::forward<ARGS>(args)...);  // Create our task which binds the functions parameters
        auto result = _function->get_future(); // Get the future of our async task for later use
        auto i = Index++;// Increases the first thread we test by one each call ensuring better work distribution

        int Attempts = 5;// Ensure fair work distribution
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
#endif
}; // End ThreadPool Class


#pragma warning( pop )
#endif// THREADPOOL_H



 /*
==========================================================================================================================================================================
                                                           NOTES:
==========================================================================================================================================================================
*/