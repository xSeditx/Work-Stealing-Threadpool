___
# Thread Pool Module
___

___
# General Purpose Work stealing schedular <br>
___

	 Handles the Acceptance and dispatching of Asynchronous Function         <br>
  calls via Threaded Queues which store pointers to functions and			 <br>
  systematically keeps every core of the CPU busy at all times. As soon		 <br>
  as one function returns another is popped from a stack and ran and the	 <br>
  user synchronizes these efforts via a future object which becomes valid	 <br>
  with the return value after the desired function has been properly		 <br>
  run.
  
___

## Resources use to aid in development of this Library                                                                  <br>          
																														<br>
### Co-Routine Specs																									<br>
http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0057r2.pdf														<br>
																														<br>
### Executors proposal																									<br>
https://github.com/chriskohlhoff/executors																				<br>
																														<br>
### Very similar work stealer to this one.																				<br>
https://codereview.stackexchange.com/questions/169377/work-stealing-queue												<br>
																														<br>
### Parallelizing the Naughty Dog Engine																				<br>
https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine												<br>
																														<br>
### CppCon 2015: Fedor Pikus PART 2 Live Lock-Free or Deadlock (Practical Lock-free Programming) Queue				  <	 br>
https://www.youtube.com/watch?v=1obZeHnAwz4&t=3055s																		<br>
																														<br>
### Code overview - Thread Pool & Job System																			<br>
https://www.youtube.com/watch?v=Df-6ws_EZno																				<br>
																														<br>
### Better Code Concurrency																								<br>
https://www.youtube.com/watch?v=zULU6Hhp42w&list=PLl8Qc2oUMko_FMAaK7WY4ly0ikLFrYCE3&index=4								<br>
																														<br>
### GotW #95: Thread Safety and Synchronization																			<br>
https://herbsutter.com/2014/01/06/gotw-95-thread-safety-and-synchronization/											<br>
																														<br>
### Thread Pool Implementation on Github:																				<br>
https://github.com/mtrebi/thread-pool/blob/master/README.md#queue														<br>
																														<br>
### Thread pool implementation using c++11 threads																		<br>
https://github.com/mtrebi/thread-pool																					<br>
																														<br>
### Faster STD::FUNCTION Implementation																					<br>
https://github.com/skarupke/std_function/blob/master/function.h															<br>
																														<br>
### Lock Free Ring Buffer																								<br>
https://github.com/tempesta-tech/blog/blob/master/lockfree_rb_q.cc														<br>
																														<br>
### Learning C++																										<br>
https://riptutorial.com/Download/cplusplus.pdf																			<br>
																														<br>
### Performance Analysis of Multithreaded Sorting Algorithms															<br>
http://www.diva-portal.org/smash/get/diva2:839729/FULLTEXT02															<br>
																														<br>
### Double Check Locking is Fixed																						<br>
https://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/													<br>
																														<br>
### Intel Game Engine Design:																							<br>
https://software.intel.com/en-us/articles/designing-the-framework-of-a-parallel-game-engine								<br>
																														<br>
### Programming with Threads																							<br>
### Parallel Sorting																									<br>
https://cseweb.ucsd.edu/classes/fa13/cse160-a/Lectures/Lec02.pdf														<br>
																														<br>
### Lock-Free Programming																								<br>
https://www.cs.cmu.edu/~410-s05/lectures/L31_LockFree.pdf																<br>
																														<br>
### Introduction to Multithreaded Algorithms																			<br>
http://ccom.uprrp.edu/~ikoutis/classes/algorithms_12/Lectures/MultithreadedAlgorithmsApril23-2012.pdf					<br>
																														<br>
### A Fast Lock-Free Queue for C++																						<br>
http://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++															<br>
																														<br>
### A Thread Pool with C++11																							<br>
http://progsch.net/wordpress/?p=81																						<br>
																														<br>
### C++11 threads, affinity and hyperthreading																			<br>
https://eli.thegreenplace.net/2016/c11-threads-affinity-and-hyperthreading/												<br>
																														<br>
### Thread pool worker implementation																					<br>
https://codereview.stackexchange.com/questions/60363/thread-pool-worker-implementation									<br>
																														<br>
### C++11 Multithreading  Part 8: std::future , std::promise and Returning values from Thread							 br><
https://thispointer.com/c11-multithreading-part-8-stdfuture-stdpromise-and-returning-values-from-thread/				<br>
																														<br>
### Threadpool with documentation:																						<br>
https://www.reddit.com/r/cpp/comments/9lvji0/c_threadpool_with_documentation/											<br>
																														<br>
### Original White paper on Work stealing Queues:																		<br>
http://supertech.csail.mit.edu/papers/steal.pdf																			<br>
																														<br>
### Type Traits Reference																								<br>
https://code.woboq.org/llvm/libcxx/include/type_traits.html																<br>
																														<br>
### MSVC Threadpool implementation for Concurrency:																		<br>
https://docs.microsoft.com/en-us/cpp/parallel/concrt/task-scheduler-concurrency-runtime?view=vs-2019					<br>
																														<br>
### Simple Threadpool Implementation:																					<br>
https://riptutorial.com/cplusplus/example/15806/create-a-simple-thread-pool												<br> 
																														<br> 
### Good Coding video website. 																							<br> 
https://channel9.msdn.com/Browse/AllContent																				<br> 