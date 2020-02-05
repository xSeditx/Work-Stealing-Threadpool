#include"Threadpool.h"
#include<assert.h>

int StaticCounter; 
int TestRecursion(int _counter);
int TestAsyncSquared(int _in);
int Test_myFuture();
int MimicMergeSort(int _value);


/* ===============================================================================================================================================
/*     SYNTAX:     Threadpool::get().Async( Function, Arguments...); 
/* ===============================================================================================================================================*/



/* ===============================================================================================================================================
/*                    ISSUES:
/*  
/*  error C2439 : 'Threadpool::asyncTask<int (__cdecl &)(int),int &>::Function' : member could not be initialized type 
/*  of Error will appear if the user is attempting to pass an R value without using std::move due partly to Const as well
/*  as my inferior means of dealing with Arguments. It will be fixed in the future or you could do it and tell me how ;)
/*  
/*  
/*  2) I have 8 cores meaning 8 open threads and Queues at any given time Increasing this to 9 leads to this form of 
/*  Child stealing threadpool to lock up as it is dependent on functions that can never run.
/*  
/* ===============================================================================================================================================*/

int RecursionLevel = 80;


int main()
{
	std::cout << "Main Thread: " << std::this_thread::get_id() << " \n"; 
//	Print(std::this_thread::get_id());
	auto A = Threadpool::get().Async(MimicMergeSort, std::move( RecursionLevel));
// Threadpool::get().Async(TestRecursion, std::move(RecursionLevel)); // Recursive function with other functions in the Pool also will lock up the TP prematurely
////auto Math = Threadpool::get().Async(TestAsyncSquared, std::move(1));
//auto FutureTest = Threadpool::get().Async(Test_myFuture);
//auto status = FutureTest.wait_for(std::chrono::seconds(10));
//Print((int)status);
//
while (Threadpool::get().is_Alive())  
    {/* Just something to prevent premature shut down while we test the Threadpool */}
    return 0;
}

/* Just a couple of the most basic test I just whipped together just so show functions being called. Create some of your own
/* to get a better idea how it all functions, aside from the need to use std::move on params which are R values you would use it 
/* No differently than one would make a basic function call or std::async as it is a direct dropin for std::async */
int impl_TestAsyncSquared(int _in)
{
    return _in * _in;
}
int TestAsyncSquared(int _in)
{
    /* Create an Array of Futures */
#ifdef _EXPERIMENTAL
	Future<int> *Result = new Future<int>[_in];
#else
	std::future<int> *Result = new std::future<int>[_in];
#endif

    for (int i{ 0 }; i < _in; ++i)
    {
         Result[i] = Threadpool::get().Async(impl_TestAsyncSquared, std::move(i));
    }
    int Total{ 0 };
    for (int i{ 0 }; i < _in; ++i)
    {
        Total += Result[i].get();
    }
    return Total;
}

int TestRecursion(int _counter)
{
//    Print("TestRecursion iteration :" << _counter);
    if (--_counter > 0)
    {
        int Param = _counter;
        auto E = Threadpool::get().Async(TestRecursion, std::move( Param));
        return E.get();
    }
 //   Print("Exiting TestRecursion: " << StaticCounter);
    Threadpool::get().Terminate();
    return StaticCounter++;
}



int Test_myFuture()
{
//	while(1){}
	return rand() % 1000;
}


/* Hmmm Intrinsic CPU Timer... Nice
  i = __rdtsc();
  
  Reads a Register
  __int64 __readmsr(
   int register
);*/

#include<stdio.h>
int MimicMergeSort(int _value)
{
	std::cout << "Merge Value: " << _value;
	if (_value < 2)
	{
		return _value;
	}
	int Half = (_value / 2);
	std::cout << "Launching A from Merge "<< _value;
	auto A = Threadpool::get().Async(MimicMergeSort, std::move(Half));
//	std::printf("Launching B from Merge ");
	auto B = Threadpool::get().Async(MimicMergeSort, std::move(Half));

//	std::cout << "Getting A " << _value;
	auto C = A.get();

//	std::cout << "Getting B " << _value;
	auto D = B.get();

	return C + D;
}












//
//
//int (__cdecl *const )(int &)'
//int (__cdecl &)(uint32_t)