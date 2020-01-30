
___
# Future 
___
___
## template<_Ty> struct Future<_Ty>
### : public SharedState<_Ty>
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


    Type     |   Value Type
-------------|------------------
using BaseClass | SharedState<_Ty>

 
