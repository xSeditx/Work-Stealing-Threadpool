
___
# Future 
___
___
## template \<_Ty \> struct Future \< _Ty \>
### : public SharedState \< _Ty \>
___

___
#### using BaseClass = SharedState\< T \>
___



  Method           |  Description
-------------------|-----------------
Future() noexcept  |  Default Future
~Future() noexcept | Destructor
Future(Future&& _other) noexcept | Moves _other into this Location 
Future(const BaseClass& _state, _PlaceHolder) | Allows us to CopyConstruct our Shared state without copying Future 
Future& operator=(Future&& _rhv) noexcept | Assignment by moving _rhv into this 
_Ty get() |  Retreives the Value set by our Promise on completion 
Future(const Future&) | delete
Future& operator=(const Future&) | delete



<br>
<br>
<br>
-<br>


 ___
 ## template \< typename _Ty \> struct Promise
 ___


  Method    |     Description
--------------|---------------------
Promise() |	 Default Constructor 
Promise(Promise&& _other) noexcept |	 Constructs by moving _Other to this location  
Promise& operator=(Promise&& _other) noexcept | Assigns _other to this Promise  
~Promise() noexcept | /* Destroys our Object  
void Swap(Promise& _other) noexcept | Swaps other promise with this one  
[[nodiscard]] Future<_Ty> get_future() | Our Associated Future<> Object  
void set_value(const _Ty& _value) | Sets the value of our Promise  
void set_value(_Ty&& _value) | Sets the value of our Promise by forwarding value to it  
Promise(const Promise&) | delete
Promise& operator = (const Promise&) | delete




___
### Data Members
___

   Data Member |     Description
-------------|---------------
SharedState<_Ty> _MyPromise; | My Shared State Object for managing the Future Results of the promise



