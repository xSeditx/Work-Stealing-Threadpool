#pragma once
#include<string>
#include<chrono>
#include<stack>
#include<atomic>
#include<mutex>

#define _EXPERIMENTAL

#define CON_Red
#define CON_Green
#define CON_Yellow
#define CON_Blue
// DEBUGPrint(col,"\n Message: " << x);// Tracer Tra(this,__FUNCTION__ )
#define DPrint(col, x) //std::cout << x << "\n" 
#define Print(x) std::cout << x << "\n" 


extern std::stack<std::string> CS;
template<typename _Ty>
struct Tracer
{
	Tracer(_Ty* _callingobject, std::string _name)
		:
		Type(typeid(*_callingobject).name()),
		Object(_callingobject),
		Function(_name),
		Address(*((uint64_t*)&_callingobject))//
	{
		Print("\n Enter " + _name + " \n Type: " << Type << "\n Object: " << Address);
		CS.push("Object: " + std::to_string(Address) + " Enter " + _name);
	}
	~Tracer()
	{
		Print("\n Exit " << Function << " \n Type: " << Type << " \n Object: " << Address);
		CS.push({ "Object: " + std::to_string(Address) + " Exit " + Function });
	}
	std::string Function;
	std::string Type;
	uint64_t Address;
	_Ty* Object;
};




struct _PlaceHolder {};
using Lock_t = std::unique_lock<std::mutex>;

enum class future_status
{ // names for timed wait function returns
	ready,
	timeout,
	deferred
};
template<typename _Ty>
void Deleter(_Ty* _object)
{
	delete *_object;
}

/*
 * ===============================================================================
 *                        State<_Ty>
 *     Container for an Asynchronous Shared state
 *     Values are Created and Stored Via Promise<_Ty>
 *     Values Retrieved using a Future<_Ty>
 * ===============================================================================
*/
template<typename _Ty>
struct State
{
	struct testReady
	{ // Functor Test for SharedState object
		testReady(const State* _St)
			: tempState(_St)
		{}
		bool operator()() const
		{
			return tempState->Ready != 0;
		}
		const State* tempState;
	};


	using State_type = _Ty;
	State() noexcept
		:
		//atomicReferenceCount(1), // non-atomic initialization
		RefCount(1),
		Retrieved(false), Ready(false), Ready_at_Thread_Exit(false), hasResult(false),
		Running(false)
	{
		DPrint(CON_Green, "Creating State Default CTOR");
		Print("Creating State of " << typeid(_Ty).name());
	}
	virtual ~State() noexcept
	{
		DPrint(CON_Red, "State dtor");
		Print("Destructor State of " << typeid(_Ty).name() << " Refcount of " << RefCount);

		if (hasResult && !Ready)
		{
			CV._Unregister(Mtx);
		}
	}

	//===================== REFERENCE COUNTER ======================================

	void _Retain()
	{ // increment reference count
		Print("Retaining State " << typeid(_Ty).name());
		++RefCount;	
	}
	void _Release()
	{ // decrement reference count and destroy when zero
		Print( "_Releasing State via Release " << typeid(_Ty).name());
		if (--RefCount == 0)
		{
			Print( "State Should be Deleted " << typeid(_Ty).name() );//_Delete_this();
		}
	}

	
	std::atomic<uint32_t> RefCount{ 0 };//uint64_t atomicReferenceCount; <- STL code

	//===============================================================================
	//====================== Suspending =============================================

	void wait()
	{
		Lock_t Lock(Mtx);
		RunDeferred();/// This is where I can Implement part of the Call chain
		while (!Ready)
		{
			CV.wait(Lock);
		}
	}

	template<class _Rep,class _Period>
	future_status wait_for(const std::chrono::duration<_Rep, _Period>& _relativeTime)
	{	// wait for duration
		Lock_t Lock(Mtx);
		if (hasDeferredFunction())
		{
			return (future_status::deferred);
		}
		if (CV.wait_for(Lock, _relativeTime, testReady(this)))
		{
			return (future_status::ready);
		}
		return (future_status::timeout);
	}

	template<class _Clock,	class _Duration>
	future_status wait_until(const std::chrono::time_point<_Clock, _Duration>& _absoluteTime)
	{	// wait until time point
		Lock_t Lock(Mtx);
		if (hasDeferredFunction())
		{//Has_deferred_function())
			return (future_status::deferred);
		}
		if (CV.wait_until(Lock, _absoluteTime, testReady(this)))
		{
			return (future_status::ready);
		}
		return (future_status::timeout);
	}



	//===============================================================================

	//==================== GET && SET ===============================================

	void swap(State<_Ty>& _other)
	{
		std::swap(*this, _other);
	}

	_Ty& get_value()
	{
		Print("Retrieving State " << typeid(_Ty).name() << " value");
		Lock_t Lock(Mtx);
		if (getOnce && Retrieved)
		{
			Print("Error:  Attempting to retrieve SharedState multiple times ");
			__debugbreak();
		}

		Retrieved = true;
		CheckDeferredFunction(Lock);
		while (!Ready)
		{
			CV.wait(Lock);
		}
		return Value;
	}
	void set_value(const _Ty& _val)
	{ // store a result
		Print("Setting State " << typeid(_Ty).name() << " value");

		Lock_t Lock(Mtx);
		set_raw_value(_val, &Lock);
	}
	void set_raw_value(const _Ty& _val, Lock_t* _lock) {
		if (hasResult)
		{
			Print("Value for Promise already set");
			__debugbreak();
		}
		Value = _val;
		Notify(_lock);
	}
	void set_value(const _Ty&& _val)
	{ // store a result
		Lock_t Lock(Mtx);
		set_raw_value(std::forward<_Ty>(_val), &Lock);
	}
	void set_raw_value(const _Ty&& _val, Lock_t* _lock) {
		if (hasResult)
		{
			Print("Value for Promise already set");
			__debugbreak();
		}
		Value = std::forward<_Ty>(_val);
		Notify(_lock);
	}
	//===============================================================================

	//====================== SIGNALS ================================================

	virtual void Notify(Lock_t* _lock)
	{ // notify waiting threads
		///CV._Register(*_lock, (int*)&Ready); when we setup at thread exit

		hasResult = true;
		Ready = true;
		CV.notify_all();
	}
	bool isReady() { return Ready; }
	bool alreadyRetrieved() { return Retrieved; }
	bool isReadyatThreadExit() { return Ready_at_Thread_Exit; }

	void makeReady() { Ready = true; }
	void makeReadyatThreadExit() { Ready_at_Thread_Exit = true; }
	//===============================================================================

	//================= BEHAVIOUR && MEMORY =========================================
	static State<_Ty>& make_New() { DPrint(CON_Green, "make_New() called in State"); return new State<_Ty>(); }

	virtual void RunDeferred(Lock_t&) { }// Print("This is where we might be able to place call chains or co-routine like functions"); }// Derive behavior
	virtual bool hasDeferredFunction() const noexcept
	{// Not sure how exactly I want to implement this so for now I will model after STL
		return (hasDeferred);
	}
	void CheckDeferredFunction(Lock_t& _lock)
	{
		if (!Running)
		{ // run the function
			Running = true;
			RunDeferred(_lock);
		}
	}


	void Abandon()
	{
		Lock_t _Lock{ Mtx };
		if (!hasResult)
		{
			Print("Error: Deleting State before function has returned");
			__debugbreak();
		}
	}
	void DeleteThis()
	{
		DPrint(CON_Red, "Delete This called from State");
		delete this;
	}
	//===============================================================================

	_Ty Value;

	std::mutex Mtx;
	std::condition_variable CV;

	bool Retrieved{ false };
	bool Ready{ false };// int if i wish to have multiple states for ready or perhaps an enum that can be converted to an int
	bool Ready_at_Thread_Exit{ false };
	bool hasResult;
	bool Running;
	bool getOnce{ false };

	// Not sure if I want to do it like this yet....
	bool hasDeferred{ false };

	State(const State&) = delete;
	State& operator=(const State&) = delete;

};




/*
 * ===============================================================================
 *                        SharedState<_Ty>
 *     Helper class which Manages the creation, Movement and Destruction of
 *     our Promise/Future State
 * ===============================================================================
*/
template<typename _Ty>
struct SharedState
{
	std::mutex Mtx;

	/* Default construction */
	SharedState()
		:
		MyState(nullptr),
		getOnce(false)
	{
		DPrint(CON_DarkGreen, "Shared State CTOR");
	}
	/* Construct from a State */
	SharedState(State<_Ty>* _new_State)
		:
		MyState(_new_State),
		getOnce(false)
	{
		DPrint(CON_Green, "Shared State from State pointer");
	}
	/* Construct my Copying anothers State */
	SharedState(const SharedState& _other)// was const SharedState&
	{
		DPrint(CON_Blue, "Shared State copy ctor");
		Copy(_other);
		getOnce = true;
	}
	/* Construct by Moving a State to this Location */
	SharedState(SharedState&& _other)// was const SharedState&
		:
		MyState(nullptr)
	{
		DPrint(CON_Blue, "Shared State move ctor");
		Move(_other);
		getOnce = true;
	}
	/* Destructor Releases Reference to this State */
	virtual ~SharedState() noexcept
	{
		DPrint(CON_Red, "Shared State destructor");
		Lock_t Lock(Mtx);
		if (MyState)
		{
			MyState->_Release();
			if (MyState->RefCount == 0)
			{
				delete MyState;
			}
		}
	}
	/* Copies other State to this one */
	SharedState& operator=(const SharedState& _other)
	{ // assign from _Other
		Copy(_other);
		return *this;
	}
	/* Assigns this state by moving it to this Location */
	SharedState& operator=(SharedState&& _other)
	{ // assign from rvalue _Other
		Move(_other);
		return *this;
	}


	//===============================================================================
	/* Test to see if the State exist and has not been Retrieved already when getOnce
	was requested */
	[[nodiscard]] bool valid()
	{
		return MyState && !(getOnce && MyState->alreadyRetrieved());
	}

	//=========================================================================== 


	//====================== SUSPENDERS =========================================

	/* Suspends Threads execution until Future State is fullfilled */
	void wait() const
	{
		if (!valid())
		{
			Print("Waiting on Future with No State");
			__debugbreak();
		}
		MyState->wait();
	}
                                                                      
	/* Wait on a Future State until a given amount of time has passed */
	template <class   _Rep, class _Period>	
	future_status wait_for(const std::chrono::duration<_Rep, _Period>& _relativeTime) 
	{	// wait for duration
		if (!this->valid())
		{// HERE
			Print("Waiting on Future with No State");//	 Make this into Error handling which only in Debug mode
			__debugbreak();
		}
		
		return (MyState->wait_for(_relativeTime));
	}

	/* Waits on a Future State until an Absolute time is reached */
	template<class _Clock,	class _Duration> 
	future_status wait_until(const std::chrono::time_point<_Clock, _Duration>& _absoluteTime) const
	{	// wait until time point
		if (!valid())
		{
			Print("Waiting on a Future with No State ");
			__debugbreak();
		}
		return (MyState->wait_until(_absoluteTime));
	}


	//===============================================================================

	//======================= GET && SET =================================================
	/* Retrieves the value Associated with our Promise */
	_Ty& get_value()
	{
		if (!valid())
		{
			Print("Error:  State Not Valid. ");
			__debugbreak();
		}
		return MyState->get_value();
	}

	/* Sets the Value in our Future State */
	void set_value(const _Ty& _val)
	{
		if (!valid()) {
			Print("No State in the Promise to set");
			__debugbreak();
		}
		MyState->set_value(_val);
	}

	/* Sets the Value in our Future State via forward reference */
	void set_value(_Ty&& _val)
	{
		if (!valid())
		{
			Print("No State in the Promise to set");
			__debugbreak();
		}
		MyState->set_value(std::forward<_Ty>(_val));
	}

	//===============================================================================
	/* Throws away this Reference to our */
	/// Properly Manage the Reference Count I believe this is incorrect
	void Abandon()
	{
		DPrint(CON_DarkRed, "Abandon called on Shared State");
		if (MyState)
		{
			MyState->Abandon();
		}
	}

	/* Get a pointer to our Shared State object */
	State<_Ty>* _Pointer() const
	{
		return MyState;
	}

	//===============================================================================


	//===============================================================================
	/* Swap two State Objects from _other preserving Reference count */
	void Swap(SharedState& _other)
	{
		std::swap(MyState, _other.MyState);
	}

	/* Copies _other State object into this Shared State Tracking our Reference */
	void Copy(const SharedState& _other)
	{
		if (this != std::addressof(_other))
		{
			if (MyState)
			{// If we hold a State Release its Reference
				MyState->_Release();
			}
			if (_other.MyState)
			{// Copy the Object
				_other.MyState->_Retain();
				MyState = _other.MyState;
				getOnce = _other.getOnce;
			}
			else
			{// If other was empty leave this empty as well
				MyState = nullptr;
			}
		}
	}

	/* Move _other State into this Container */
	void Move(SharedState& _other)
	{
		if (this != std::addressof(_other))
		{
			if (MyState)
			{// Decrement the Reference if it exist
				MyState->_Release();
			}
			MyState = _other.MyState;
			_other.MyState = nullptr;
			getOnce = _other.getOnce;
		}
	}

	//===============================================================================

	//=================== SIGNALS && STATE ==========================================
	/* Test if our Promise has set the State for our future */
	bool isReady()
	{/// Possibly minor optimization removing Test for Null State
		return MyState && MyState->isReady();
	}

	/* Tells if our Promise should be filled when our Thread Exits */
	bool isReadyatThreadExit()
	{
		return MyState->isReady_at_Thread_Exit();
	}
	//===============================================================================

	State<_Ty>* MyState{ nullptr }; // State to be Created by our Promise and monitored by our Future
	bool getOnce; // Flag to denote if value should only be aquired once
};




/*
 * ===============================================================================
 *                        Future<_Ty>
 *     Monitors a Promise Object and returns the value once our
 *     Promise has been Fullfilled
 * ===============================================================================
*/
template<typename _Ty>
struct Future
	:
	public SharedState<_Ty>
{
	using BaseClass = SharedState<_Ty>;
	/* Default Future */
	Future() noexcept
	{
		DPrint(CON_Green, "Future default ctor");
	}
	/* Destructor */
	~Future() noexcept {
		DPrint(CON_Red, "Future destructor");
	}
	/* Moves _other into this Location */
	Future(Future&& _other) noexcept
		:
		BaseClass(std::move(_other))
	{
		DPrint(CON_Blue, "Future move constructor");
	}// construct from rvalue future object
	/* Allows us to CopyConstruct our Shared state without copying Future */
	Future(const BaseClass& _state, _PlaceHolder)
		:
		BaseClass(_state)
	{ // construct from associated asynchronous state object
		DPrint(CON_Yellow, "Future from SharedState ctor");
	}
	/* Assignment by moving _rhv into this */
	Future& operator=(Future&& _rhv) noexcept
	{
		BaseClass::operator=(std::move(_rhv));
		return *this;
	}
	/* Retreives the Value set by our Promise on completion */
	_Ty get()
	{
		Future Local{ std::move(*this) };
		return std::move(Local.get_value());
	}

	Future(const Future&) = delete;
	Future& operator=(const Future&) = delete;
};




/*
 * ===============================================================================
 *                        Future<_Ty&>
 *     Monitors a Promise Object and returns the value once our
 *     Promise has been Fullfilled
 * ===============================================================================
*/
template<typename _Ty>
struct Future<_Ty&>
	:
	public SharedState<_Ty*>
{
	using BaseClass = SharedState<_Ty*>;
	/* Default Future */
	Future() noexcept {
		DPrint(CON_Green, "Future of Reference default ctor");
		State<_Ty> TempState;
	}
	/* Destructor */
	~Future() noexcept {
		DPrint(CON_Red, "Future of Reference destructor");
	}
	/* Moves Constructs from _other into this Location */
	Future(Future&& _other) noexcept
		:
		BaseClass(std::move(_other))
	{
		DPrint(CON_Blue, "Future of Reference move ctor");
	}
	/* Allows us to CopyConstruct our Shared state without copying Future */
	Future(const BaseClass& _state, _PlaceHolder)
		:
		BaseClass(_state)
	{
		DPrint(CON_Blue, "Future of Reference from SharedState constructor");
	}
	/* Constructs by moving _rhv into this */
	Future& operator=(Future&& _rhv) noexcept
	{
		BaseClass::operator=(std::move(_rhv));
		return *this;
	}
	/* Retreives the Value set by our Promise on completion */
	_Ty& get()
	{ // block until ready then return the stored result or throw the stored exception
		Future Local{ std::move(*this) };
		return *Local.get_value();
	}

	Future(const Future&) = delete;
	Future& operator=(const Future&) = delete;
};




/*
 * ===============================================================================
 *                        Promise<_Ty>
 *     Creates an Object which will contain a valid value sometime in the Future
 *     Pair with a Future<_Ty> to aquire our Promise value later on
 * ===============================================================================
*/
template<typename _Ty>
struct Promise
{
	/* Default Constructor */
	Promise()
		:
		_MyPromise(new State<_Ty>)
	{
		DPrint(CON_Green, "Promise Default ctor");
	}
	/* Constructs by moving _Other to this location */
	Promise(Promise&& _other) noexcept
		:
		_MyPromise(std::move(_other._MyPromise))
	{
		DPrint(CON_Blue, "Promise move ctor");
	}
	/* Assigns _other to this Promise */
	Promise& operator=(Promise&& _other) noexcept/*Move assignment operator. First, abandons the shared state (as in ~promise()),  then assigns the shared state of other as if by executing std::promise(std::move(other)).swap(*this).*/
	{
		Promise(std::move(_other)).Swap(*this);
		return *this;
	}
	/* Destroys our Object */
	~Promise() noexcept
	{
		DPrint(CON_Red, "Promise destructor");
		if (_MyPromise.valid() && !_MyPromise.isReady())
		{
			Print("Promise Destroyed before function returned");
			__debugbreak();
		}
	}
	/* Swaps other promise with this one */
	void Swap(Promise& _other) noexcept
	{
		_MyPromise.Swap(_other._MyPromise);
	}
	/* Our Associated Future<> Object */
	[[nodiscard]] Future<_Ty> get_future()
	{
		return Future<_Ty>(_MyPromise, _PlaceHolder());
	}
	/* Sets the value of our Promise */
	void set_value(const _Ty& _value)
	{
		_MyPromise.set_value(_value);
	}
	/* Sets the value of our Promise by forwarding value to it */
	void set_value(_Ty&& _value)
	{
		_MyPromise.set_value(std::forward<_Ty>(_value));
	}


	SharedState<_Ty> _MyPromise;

	Promise(const Promise&) = delete;
	Promise& operator = (const Promise&) = delete;
};




/*
 * ===============================================================================
 *                        Promise<_Ty>
 *     Creates an Object which will contain a valid value sometime in the Future
 *     Pair with a Future<_Ty> to aquire our Promise value later on
 * ===============================================================================
*/
template<typename _Ty>
struct Promise<_Ty&>
{
	/* Default Constructor */
	Promise()
		:
		_MyPromise(new State<_Ty*>())
	{
		DPrint(CON_Green, "Promise of Reference default ctor");
	}
	/* Constructs by moving _Other to this location */
	Promise(Promise&& _other) noexcept
		:
		_MyPromise(std::move(_other._MyPromise))
	{
		DPrint(CON_Blue, "Promise of Reference Move ctor");
	}
	/* Assigns _other to this Promise */
	Promise& operator=(Promise&& _other) noexcept/*Move assignment operator. First, abandons the shared state (as in ~promise()),  then assigns the shared state of other as if by executing std::promise(std::move(other)).swap(*this).*/
	{
		Promise(std::move(_other)).Swap(*this);
		return *this;
	}
	/* Destroys our Object */
	~Promise() noexcept
	{
		DPrint(CON_Red, "Promise of Reference destructor");
		if (_MyPromise.valid() && !_MyPromise.isReady())
		{
			Print("Promise Destroyed before function returned");
			__debugbreak();
		}
	}
	/* Swaps other promise with this one */
	void Swap(Promise& _other) noexcept
	{
		_MyPromise.Swap(_other._MyPromise);
	}
	/* Our Associated Future<> Object */
	[[nodiscard]] Future<_Ty&> get_future()
	{
		return Future<_Ty&>(_MyPromise, _PlaceHolder());
	}
	/* Sets the value of our Promise */
	void set_value(_Ty& _value)
	{
		_MyPromise.set_value(&_value);
	}

	SharedState<_Ty*> _MyPromise;

	Promise(const Promise&) = delete;
	Promise& operator = (const Promise&) = delete;
};


///======================================================================================================================================================
// ALTERNATIVE WHICH DERIVES FROM STD::PROMISE AND FUTURE FOR WRITING CO_ROUTINE LIKE FUNCTIONS
///======================================================================================================================================================

#include<future>

template<typename _Ty>
struct myFuture
	: public std::future<_Ty>
{
	using BaseClass = std::future<_Ty>;
	myFuture(BaseClass& _other)
		:
		BaseClass(_other)
	{}
	//	bool await_ready();
	//	void await_suspend(std::coroutine_handle<>);
	_Ty await_resume();
};

template<typename _Ty>
struct myPromise
	:public std::promise<_Ty>
{
	myFuture<_Ty>& get_future()
	{
		return myFuture<_Ty>(std::future<_Ty>());
	}
};

///co_await, co_yield and co_return. 

template<typename _Ty>
struct promise_type
{
	Promise<_Ty> _MyPromise;
	Future<_Ty> get_return_object() {
		return _MyPromise.get_future();
	}
	bool initial_suspend() const {
		return false;
	}
	bool final_suspend() const {
		return false;
	}
};