#include"Future.h"

//_Associated_state::wait_for and wait_until

/*
template<class _Rep,
	class _Per>
	future_status _Wait_for(
		const chrono::duration<_Rep, _Per>& _Rel_time)
{	// wait for duration
	unique_lock<mutex> _Lock(_Mtx);
	if (_Has_deferred_function())
		return (future_status::deferred);
	if (_Cond.wait_for(_Lock, _Rel_time, _Test_ready(this)))
		return (future_status::ready);
	return (future_status::timeout);
}

template<class _Clock,
	class _Dur>
	future_status _Wait_until(
		const chrono::time_point<_Clock, _Dur>& _Abs_time)
{	// wait until time point
	unique_lock<mutex> _Lock(_Mtx);
	if (_Has_deferred_function())
		return (future_status::deferred);
	if (_Cond.wait_until(_Lock, _Abs_time, _Test_ready(this)))
		return (future_status::ready);
	return (future_status::timeout);
}


SEE CORRECTION OF VOID STATES OR POINTERS et using P_arg_type
template<class _Ret,
	class _Fty> inline
	_Associated_state<typename _P_arg_type<_Ret>::type>
*/

/*




State Manager


	void wait() const
		{	// wait for signal
		if (!valid())
			_Throw_future_error(make_error_code(future_errc::no_state));
		_Assoc_state->_Wait();
		}

	template<class _Rep,
		class _Per>
		future_status wait_for(const chrono::duration<_Rep, _Per>& _Rel_time) const
		{	// wait for duration
		if (!valid())
			_Throw_future_error(make_error_code(future_errc::no_state));
		return (_Assoc_state->_Wait_for(_Rel_time));
		}

	template<class _Clock,
		class _Dur>
		future_status wait_until(const chrono::time_point<_Clock, _Dur>& _Abs_time) const
		{	// wait until time point
		if (!valid())
			_Throw_future_error(make_error_code(future_errc::no_state));
		return (_Assoc_state->_Wait_until(_Abs_time));
		}

*/



/* SET VALUES FOR STD::PROMISE


	void set_value(const _Ty& _Val)
		{	// store result
		_MyPromise._Get_state_for_set()._Set_value(_Val, false);
		}

	void set_value_at_thread_exit(const _Ty& _Val)
		{	// store result and block until thread exit
		_MyPromise._Get_state_for_set()._Set_value(_Val, true);
		}

	void set_value(_Ty&& _Val)
		{	// store result
		_MyPromise._Get_state_for_set()._Set_value(
			_STD forward<_Ty>(_Val), false);
		}

	void set_value_at_thread_exit(_Ty&& _Val)
		{	// store result and block until thread exit
		_MyPromise._Get_state_for_set()._Set_value(
			_STD forward<_Ty>(_Val), true);
		}

	void set_exception(exception_ptr _Exc)
		{	// store result
		_MyPromise._Get_state_for_set()._Set_exception(_Exc, false);
		}

	void set_exception_at_thread_exit(exception_ptr _Exc)
		{	// store result and block until thread exit
		_MyPromise._Get_state_for_set()._Set_exception(_Exc, true);
		}



*/

/* PACKAGED TASK C_TOR, Should model asynTask after this at least the enable_if to ensure proper utilization and provide more appropriate error messages

	template<class _Fty2,
		class = enable_if_t<!is_same_v<remove_cv_t<remove_reference_t<_Fty2>>, packaged_task>>>
		explicit packaged_task(_Fty2&& _Fnarg)
		: _MyPromise(new _MyStateType(_STD forward<_Fty2>(_Fnarg)))
		{	// construct from rvalue function object
		}

		GET FUTURE for PACKAGE TASK. It Creates a new Future via move semantics from the looks of it
	_NODISCARD future<_Ret> get_future()
		{	// return a future object that shares the associated
			// asynchronous state
		return (future<_Ret>(_MyPromise._Get_state_for_future(), _Nil()));
		}

*/


