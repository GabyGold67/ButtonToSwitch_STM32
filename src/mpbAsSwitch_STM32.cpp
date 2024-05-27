/**
 * @file		: mpbAsSwitch_STM32.cpp
 * @brief	: Source file for mpbAsSwitch_STM32 library classes methods
 *
 * @author	: Gabriel D. Goldman
 * @date		: Created on: 06/11/2023
 * @date		: Last modification: 05/05/2024
 */

#include "mpbAsSwitch_STM32.h"

DbncdMPBttn::DbncdMPBttn()
:_mpbttnPort{NULL}, _mpbttnPin{0}, _pulledUp{true}, _typeNO{true}, _dbncTimeOrigSett{0}
{
}

DbncdMPBttn::DbncdMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett)
: _mpbttnPort{mpbttnPort}, _mpbttnPin{mpbttnPin}, _pulledUp{pulledUp}, _typeNO{typeNO}, _dbncTimeOrigSett{dbncTimeOrigSett}
{
	if(_mpbttnPin != _InvalidPinNum){
      _mpbPollTmrName = "PollMpbPin";
      std::string mpbPinNumStr{ "00" + std::to_string(singleBitPosNum(_mpbttnPin)) };

		if(mpbttnPort == GPIOA){
			_mpbPollTmrName += "A";
			__HAL_RCC_GPIOA_CLK_ENABLE();	//Sets the bit in the GPIO enabled clocks register, by logic OR of the corresponding bit, no problem if already set, macro adds time to get the clk running
		}
		else if(mpbttnPort == GPIOB){
			_mpbPollTmrName += "B";
			__HAL_RCC_GPIOB_CLK_ENABLE();
		}
		else if(mpbttnPort == GPIOC){
			_mpbPollTmrName += "C";
			__HAL_RCC_GPIOC_CLK_ENABLE();
		}
		else if(mpbttnPort == GPIOD){
			_mpbPollTmrName += "D";
			__HAL_RCC_GPIOD_CLK_ENABLE();
		}
#ifdef GPIOE
		else if(mpbttnPort == GPIOE){
			_mpbPollTmrName += "E";
			__HAL_RCC_GPIOE_CLK_ENABLE();
		}
#endif
#ifdef GPIOF
		else if(mpbttnPort == GPIOF){	//Port not present in all STM32 MCUs/DevBoards
			_mpbPollTmrName += "F";
			__HAL_RCC_GPIOF_CLK_ENABLE();
		}
#endif
#ifdef GPIOG
		else if(mpbttnPort == GPIOG){	//Port not present in all STM32 MCUs/DevBoards
			_mpbPollTmrName += "G";
			__HAL_RCC_GPIOG_CLK_ENABLE();
		}
#endif
#ifdef GPIOH
		else if(mpbttnPort == GPIOH){	//Port not present in all STM32 MCUs/DevBoards
			_mpbPollTmrName += "H";

			__HAL_RCC_GPIOH_CLK_ENABLE();
		}
#endif
#ifdef GPIOI
		else if(mpbttnPort == GPIOI){	//Port not present in all STM32 MCUs/DevBoards
			_mpbPollTmrName += "I";
			__HAL_RCC_GPIOI_CLK_ENABLE();
}
#endif

		mpbPinNumStr = mpbPinNumStr.substr(mpbPinNumStr.length() - 2, 2);

		_mpbPollTmrName = _mpbPollTmrName + mpbPinNumStr + "_tmr";

		if(_dbncTimeOrigSett < _stdMinDbncTime) 	// Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
			_dbncTimeOrigSett = _stdMinDbncTime;	//this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
		_dbncTimeTempSett = _dbncTimeOrigSett;
		_dbncRlsTimeTempSett = _stdMinDbncTime;	//The Release debouncing time parameter is kept to the minimum empirical value

		/*Configure GPIO pin : _mpbttnPin */
      GPIO_InitTypeDef GPIO_InitStruct {0};

      GPIO_InitStruct.Pin = _mpbttnPin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = (_pulledUp == true)?GPIO_PULLUP:GPIO_PULLDOWN;
		HAL_GPIO_Init(_mpbttnPort, &GPIO_InitStruct);
	}
	else{
      _pulledUp = true;
      _typeNO = true;
      _dbncTimeOrigSett = 0;
	}
}

DbncdMPBttn::DbncdMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett)
:DbncdMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett)
{
}

DbncdMPBttn::~DbncdMPBttn() {

	// Stop the refreshing timer:
	end();
	// De-initialize the GPIOx peripheral registers to their default reset values
	HAL_GPIO_DeInit(_mpbttnPort, _mpbttnPin);
	// Disable the GPIOx_CLK:
	//	__HAL_RCC_GPIOx_CLK_DISABLE ();
	//(RCC->AHB1ENR &= ~(RCC_AHB1ENR_GPIOxEN))   //Beware, just disabling the bit corresponding to one pin

}

bool DbncdMPBttn::begin(const unsigned long int &pollDelayMs) {
    bool result {false};
    BaseType_t tmrModResult {pdFAIL};

    if (pollDelayMs > 0){
        if (!_mpbPollTmrHndl){
            _mpbPollTmrHndl = xTimerCreate(
            		_mpbPollTmrName.c_str(),  //Timer name
                pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks by conversion from milliseconds
                pdTRUE,     //Auto-reload true
                this,       //TimerID: data passed to the callback function to work
                mpbPollCallback	  //Callback function
				);
            if (_mpbPollTmrHndl != NULL){
               tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
            	if (tmrModResult == pdPASS)
                  result = true;
            }
        }
    }

    return result;
}

void DbncdMPBttn::clrStatus(bool clrIsOn){
	/*To Resume operations after a pause() without risking generating false "Valid presses" and "On" situations,
	several attributes must be reseted to "Start" values
	The only important value not reseted is the _mpbFdaState, to do it call resetFda() INSTEAD of this method*/

	_isPressed = false;
	_validPressPend = false;
	_validReleasePend = false;
	_dbncTimerStrt = 0;
	_dbncRlsTimerStrt = 0;
	if(clrIsOn){
		if(_isOn){
			_turnOff();
		}
	}

	return;
}

void DbncdMPBttn::clrSttChng(){
	_sttChng = false;

	return;
}

bool DbncdMPBttn::disable(){

    return setIsEnabled(false);
}

bool DbncdMPBttn::enable(){

    return setIsEnabled(true);
}

bool DbncdMPBttn::end(){
	bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (_mpbPollTmrHndl){
   	result = pause();
      if (result){
      	tmrModResult = xTimerDelete(_mpbPollTmrHndl, portMAX_DELAY);
			if (tmrModResult == pdPASS){
				_mpbPollTmrHndl = NULL;
			}
			else{
				result = false;
			}
      }
   }

   return result;
}

const unsigned long int DbncdMPBttn::getCurDbncTime() const{

	return _dbncTimeTempSett;
}

fncPtrType DbncdMPBttn::getFnWhnTrnOff(){

	return _fnWhnTrnOff;
}

fncPtrType DbncdMPBttn::getFnWhnTrnOn(){

	return _fnWhnTrnOn;
}

const bool DbncdMPBttn::getIsEnabled() const{

    return _isEnabled;
}

const bool DbncdMPBttn::getIsOn() const {

	return _isOn;
}

const bool DbncdMPBttn::getIsOnDisabled() const{

    return _isOnDisabled;
}

const bool DbncdMPBttn::getIsPressed() const {

	return _isPressed;
}

const bool DbncdMPBttn::getOutputsChange() const{

    return _outputsChange;
}

unsigned long int DbncdMPBttn::getStrtDelay(){

	return _strtDelay;
}

const TaskHandle_t DbncdMPBttn::getTaskToNotify() const{

    return _taskToNotifyHndl;
}

const TaskHandle_t DbncdMPBttn::getTaskWhileOn(){

	return _taskWhileOnHndl;
}

bool DbncdMPBttn::init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett){
    bool result {false};

    if (_mpbPollTmrName == ""){
		_mpbttnPin = mpbttnPin;
		_pulledUp = pulledUp;
		_typeNO = typeNO;
		_dbncTimeOrigSett = dbncTimeOrigSett;

      std::string mpbPollTmrName{ "PollMpbPin" };
      std::string mpbPinNumStr{ "00" + std::to_string(singleBitPosNum(_mpbttnPin)) };


  		if(mpbttnPort == GPIOA){
			mpbPollTmrName += "A";
  			__HAL_RCC_GPIOA_CLK_ENABLE();	//Sets the bit in the GPIO enabled clocks register, by logic OR of the corresponding bit, no problem if already set, macro adds time to get the clk running
  		}
  		else if(mpbttnPort == GPIOB){
			mpbPollTmrName += "B";
  			__HAL_RCC_GPIOB_CLK_ENABLE();
  		}
  		else if(mpbttnPort == GPIOC){
  			mpbPollTmrName += "C";
  			__HAL_RCC_GPIOC_CLK_ENABLE();
  		}
  		else if(mpbttnPort == GPIOD){
  			mpbPollTmrName += "D";
  			__HAL_RCC_GPIOD_CLK_ENABLE();
  		}
  		else if(mpbttnPort == GPIOE){
  			mpbPollTmrName += "E";
  			__HAL_RCC_GPIOE_CLK_ENABLE();
  		}
  #ifdef GPIOF
  		else if(mpbttnPort == GPIOF){	//Port not present in all STM32 MCUs/DevBoards
  			mpbPollTmrName += "F";
  			__HAL_RCC_GPIOF_CLK_ENABLE();
  		}
  #endif
  #ifdef GPIOG
  		else if(mpbttnPort == GPIOG){	//Port not present in all STM32 MCUs/DevBoards
  			mpbPollTmrName += "G";
  			__HAL_RCC_GPIOG_CLK_ENABLE();
  		}
  #endif
  #ifdef GPIOH
  		else if(mpbttnPort == GPIOH){	//Port not present in all STM32 MCUs/DevBoards
  			mpbPollTmrName += "H";
  			__HAL_RCC_GPIOH_CLK_ENABLE();
  		}
  #endif
  #ifdef GPIOI
  		else if(mpbttnPort == GPIOI){	//Port not present in all STM32 MCUs/DevBoards
  			mpbPollTmrName += "I";
  			__HAL_RCC_GPIOI_CLK_ENABLE();
  }
  #endif

		mpbPinNumStr = mpbPinNumStr.substr(mpbPinNumStr.length() - 2, 2);

		_mpbPollTmrName = _mpbPollTmrName + mpbPinNumStr + "_tmr";

        if(_dbncTimeOrigSett < _stdMinDbncTime) 	// Best practice would impose failing the constructor (throwing an exeption or building a "zombie" object)
            _dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
        _dbncTimeTempSett = _dbncTimeOrigSett;
        _dbncRlsTimeTempSett = _stdMinDbncTime;	//The Release debouncing time parameter is kept to the minimum empirical value

        GPIO_InitTypeDef GPIO_InitStruct = {0};
    		/*Configure GPIO pin : tstMPBttn_Pin */
    		GPIO_InitStruct.Pin = _mpbttnPin;
    		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    		GPIO_InitStruct.Pull = (_pulledUp == true)?GPIO_PULLUP:GPIO_PULLDOWN;
    		HAL_GPIO_Init(_mpbttnPort, &GPIO_InitStruct);
			result = true;
    }

    return result;
}

bool DbncdMPBttn::init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett){

	return init(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett);
}

void DbncdMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	DbncdMPBttn *mpbObj = (DbncdMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	if(mpbObj->getIsEnabled()){
		// Input/Output signals update
		mpbObj->updIsPressed();
		// Flags/Triggers calculation & update
		mpbObj->updValidPressesStatus();
	}

	// State machine status update
	mpbObj->updFdaState();

	if (mpbObj->getOutputsChange()){	//Output changes might happen as part of the Disable/Enable process or setting/clearing some flags even if the MPB is disabled...
		if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
		mpbObj->setOutputsChange(false);
	 }

    return;
}

bool DbncdMPBttn::pause(){
    bool result {false};
    BaseType_t tmrModResult {pdFAIL};

    if (_mpbPollTmrHndl){
   	 if (xTimerIsTimerActive(_mpbPollTmrHndl)){
   		 tmrModResult = xTimerStop(_mpbPollTmrHndl, portMAX_DELAY);
   		 if (tmrModResult == pdPASS)
   			 result = true;
   	 }
    }

    return result;
}

bool DbncdMPBttn::resetDbncTime(){

    return setDbncTime(_dbncTimeOrigSett);
}

bool DbncdMPBttn::resetFda(){
	clrStatus();
	setSttChng();
	_mpbFdaState = stOffNotVPP;

	return true;
}

bool DbncdMPBttn::resume(){
	bool result {false};
	BaseType_t tmrModResult {pdFAIL};

	resetFda();	//To restart in a safe situation the FDA is resetted to have all flags and timers cleaned up

	if (_mpbPollTmrHndl){
		if (xTimerIsTimerActive(_mpbPollTmrHndl) == pdFAIL){	// This enforces the timer to be stopped to let the timer be resumed, makes the method useless just to reset the timer counter
			tmrModResult = xTimerReset( _mpbPollTmrHndl, portMAX_DELAY);
			if (tmrModResult == pdPASS)
				result = true;
		}
	}

	return result;
}

bool DbncdMPBttn::setDbncTime(const unsigned long int &newDbncTime){
    bool result {false};

    if(_dbncTimeTempSett != newDbncTime){
		 if (newDbncTime >= _stdMinDbncTime){
			  _dbncTimeTempSett = newDbncTime;
			  result = true;
		 }
    }

    return result;
}

bool DbncdMPBttn::setFnWhnTrnOffPtr(void (*newFnWhnTrnOff)()){
	bool result{false};

	if (_fnWhnTrnOff != newFnWhnTrnOff){
		_fnWhnTrnOff = newFnWhnTrnOff;
		result = true;
	}

	return result;
}

bool DbncdMPBttn::setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)()){
	bool result{false};

	if (_fnWhnTrnOn != newFnWhnTrnOn){
		_fnWhnTrnOn = newFnWhnTrnOn;
		result = true;
	}

	return result;
}

bool DbncdMPBttn::setIsEnabled(const bool &newEnabledValue){
	bool result{false};

	if(_isEnabled != newEnabledValue){
		if (newEnabledValue){  //Change to Enabled = true
			_validEnablePend = true;
			if(_validDisablePend)
				_validDisablePend = false;
		}
		else{	//Change to Enabled = false  (i.e. Disabled)
			_validDisablePend = true;
			if(_validEnablePend)
				_validEnablePend = false;
		}
		result = true;
	}

	return result;
}

bool DbncdMPBttn::setIsOnDisabled(const bool &newIsOnDisabled){
	if(_isOnDisabled != newIsOnDisabled){
		_isOnDisabled = newIsOnDisabled;
		if(!_isEnabled){
			if(_isOn != _isOnDisabled){
				if(_isOnDisabled){
					_turnOn();
				}
				else{
					_turnOff();
				}
			}
		}
	}

	return _isOnDisabled;
}

bool DbncdMPBttn::setOutputsChange(bool newOutputsChange){
   if(_outputsChange != newOutputsChange)
   	_outputsChange = newOutputsChange;

   return _outputsChange;
}

void DbncdMPBttn::setSttChng(){
	_sttChng = true;

	return;
}

bool DbncdMPBttn::setTaskToNotify(TaskHandle_t newHandle){
    bool result {true};

	 if(_taskToNotifyHndl != newHandle)
		 _taskToNotifyHndl = newHandle;
    if (newHandle == NULL)
        result = false;

    return result;
}

bool DbncdMPBttn::setTaskWhileOn(const TaskHandle_t &newTaskHandle){
	bool result {false};
	eTaskState taskWhileOnStts{};

	if(_taskWhileOnHndl != newTaskHandle){
		if (newTaskHandle == NULL){
			taskWhileOnStts = eTaskGetState(_taskWhileOnHndl);
			if (taskWhileOnStts != eSuspended){
				if(taskWhileOnStts != eDeleted){
					vTaskSuspend(_taskWhileOnHndl);
					_taskWhileOnHndl = NULL;
				}
				result = true;
			}
		}
		else{
			_taskWhileOnHndl = newTaskHandle;
			result = true;
		}
	}

	return result;
}

void DbncdMPBttn::_turnOff(){
	if(_isOn){
		//---------------->> Flags related actions
		_isOn = false;
		_outputsChange = true;
		//---------------->> Tasks related actions
		if(_taskWhileOnHndl != NULL){
			eTaskState taskWhileOnStts{eTaskGetState(_taskWhileOnHndl)};
			if (taskWhileOnStts != eSuspended){
				if(taskWhileOnStts != eDeleted){
					vTaskSuspend(_taskWhileOnHndl);
				}
			}
		}
		//---------------->> Functions related actions
		if(_fnWhnTrnOff != nullptr){
			_fnWhnTrnOff();
		}
	}

	return;
}

void DbncdMPBttn::_turnOn(){
	if(!_isOn){
		//---------------->> Flags related actions
		_isOn = true;
		_outputsChange = true;
		//---------------->> Tasks related actions
		if(_taskWhileOnHndl != NULL){
			eTaskState taskWhileOnStts{eTaskGetState(_taskWhileOnHndl)};
			if(taskWhileOnStts != eDeleted){
				if (taskWhileOnStts == eSuspended){
					vTaskResume(_taskWhileOnHndl);
				}
			}
		}
		//---------------->> Functions related actions
		if(_fnWhnTrnOn != nullptr){
			_fnWhnTrnOn();
		}
	}

	return;
}

void DbncdMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_isOn){
				_turnOn();
			}
			_validPressPend = false;
			_mpbFdaState = stOn;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOn:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_isOn){
				_turnOff();
			}
			_validReleasePend = false;
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				if(_isOn != _isOnDisabled){
					if(_isOn){
						_turnOff();
					}
					else{
						_turnOn();
					}
				}
				clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected
				_isEnabled = false;
				if(!_outputsChange)
					_outputsChange = true;
				_validDisablePend = false;
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				if(_isOn){
					_turnOff();
				}
				_isEnabled = true;
				_validEnablePend = false;
				if(!_outputsChange)
					_outputsChange = true;
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
			}	// Execute this code only ONCE, when exiting this state
			break;

	default:
		break;
	}

	return;
}

bool DbncdMPBttn::updIsPressed(){
    /*To be 'pressed' the conditions are:
    1) For NO == true
        a)  _pulledUp == false ==> digitalRead == HIGH
        b)  _pulledUp == true ==> digitalRead == LOW
    2) For NO == false
        a)  _pulledUp == false ==> digitalRead == LOW
        b)  _pulledUp == true ==> digitalRead == HIGH
    */
    bool result {false};
    bool tmpPinLvlSet {false};

    if(HAL_GPIO_ReadPin(_mpbttnPort, _mpbttnPin) == GPIO_PIN_SET)
   	 tmpPinLvlSet = true;

    if (_typeNO == true){
        //For NO MPBs
        if (_pulledUp == false){
            if (tmpPinLvlSet == true)
                result = true;
        }
        else{
            if (tmpPinLvlSet == false)
                result = true;
        }
    }
    else{
        //For NC MPBs
        if (_pulledUp == false){
            if (tmpPinLvlSet == false)
                result = true;
        }
        else{
            if (tmpPinLvlSet == true)
                result = true;
        }
    }
    _isPressed = result;

    return _isPressed;
}

bool DbncdMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(_dbncRlsTimerStrt != 0)
			_dbncRlsTimerStrt = 0;
		if(!_prssRlsCcl){
			if(_dbncTimerStrt == 0){    //This is the first detection of the press event
				_dbncTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be pressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
					_validPressPend = true;
					_validReleasePend = false;
					_prssRlsCcl = true;
				}
			}
		}
	}
	else{
		if(_dbncTimerStrt != 0)
			_dbncTimerStrt = 0;
		if(_prssRlsCcl){
			if(_dbncRlsTimerStrt == 0){    //This is the first detection of the release event
				_dbncRlsTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be UNpressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
					_validReleasePend = true;
					_prssRlsCcl = false;
				}
			}
		}
	}

	return (_validPressPend||_validReleasePend);
}

//=========================================================================> Class methods delimiter

DbncdDlydMPBttn::DbncdDlydMPBttn()
:DbncdMPBttn()
{
}

DbncdDlydMPBttn::DbncdDlydMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DbncdMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett)
{
	 _strtDelay = strtDelay;
}

DbncdDlydMPBttn::DbncdDlydMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DbncdDlydMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool DbncdDlydMPBttn::init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay){
    bool result {false};

    result = DbncdMPBttn::init(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett);
    if (result)
        result = setStrtDelay(strtDelay);

    return result;
}

bool DbncdDlydMPBttn::init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay){

	return init(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay);
}

bool DbncdDlydMPBttn::setStrtDelay(const unsigned long int &newStrtDelay){
	bool result {false};

    if(_strtDelay != newStrtDelay){
        _strtDelay = newStrtDelay;
        result = true;
    }

    return result;
}

//=========================================================================> Class methods delimiter

LtchMPBttn::LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DbncdDlydMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

LtchMPBttn::LtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool LtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
   	if (!_mpbPollTmrHndl){
   		_mpbPollTmrHndl = xTimerCreate(
   				_mpbPollTmrName.c_str(),  //Timer name
				pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks
				pdTRUE,     //Auto-reload true
				this,       //TimerID: data passed to the callback function to work
				mpbPollCallback
			);
         if (_mpbPollTmrHndl != NULL){
            tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
         	if (tmrModResult == pdPASS)
               result = true;
         }
   	}
	}

	return result;
}

void LtchMPBttn::clrStatus(bool clrIsOn){
	_isLatched = false;
	_validUnlatchPend = false;
	_validUnlatchRlsPend = false;
	DbncdMPBttn::clrStatus(clrIsOn);

	return;
}

const bool LtchMPBttn::getIsLatched() const{

	return _isLatched;
}

const bool LtchMPBttn::getUnlatchPend() const{

	return _validUnlatchPend;
}

bool LtchMPBttn::getTrnOffASAP(){

	return _trnOffASAP;
}

bool LtchMPBttn::setTrnOffASAP(const bool &newVal){
	if(_trnOffASAP != newVal){
		_trnOffASAP = newVal;
	}

	return _trnOffASAP;
}

bool LtchMPBttn::setUnlatchPend(const bool &newVal){
	if(_validUnlatchPend != newVal)
		_validUnlatchPend = newVal;

	return _validUnlatchPend;
}

bool LtchMPBttn::setUnlatchRlsPend(const bool &newVal){
	if(_validUnlatchRlsPend != newVal)
		_validUnlatchRlsPend = newVal;

	return _validUnlatchRlsPend;
}

bool LtchMPBttn::unlatch(){
	if(_isLatched){
		_validUnlatchPend = true;
		_validUnlatchRlsPend = true;
	}

	return _isLatched;
}

void LtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
				stOffNotVPP_In();
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	// For this stDisabled entry, the only flags that might be affected are _ validPressPend and (unlikely) _validReleasePend
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOffNotVPP_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_isOn){
				_turnOn();
			}
			_validPressPend = false;
			_mpbFdaState = stOnNVRP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOffVPP_Out();	//This function starts the latch timer here... to be considered if the MPB release must be the starting point Gaby
			}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnNVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			stOnNVRP_Do();
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}

			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validReleasePend = false;
			if(!_isLatched)
				_isLatched = true;
			_mpbFdaState = stLtchNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stLtchNVUP:	//From this state on different unlatch sources might make sense
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			stLtchNVUP_Do();
			if(_validUnlatchPend){
				_mpbFdaState = stLtchdVUP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stLtchdVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_trnOffASAP){
				if(_isOn){
					_turnOff();
				}
			}
			_mpbFdaState = stOffVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validUnlatchPend = false;	// This is a placeholder for updValidUnlatchStatus() implemented in each subclass
			_mpbFdaState = stOffNVURP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffNVURP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validUnlatchRlsPend){
				_mpbFdaState = stOffVURP;
				setSttChng();
			}
			stOffNVURP_Do();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVURP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validUnlatchRlsPend = false;
			if(_isOn){
				_turnOff();
			}
			if(_isLatched)
				_isLatched = false;
			if(_validPressPend)
				_validPressPend = false;
			if(_validReleasePend)
				_validReleasePend = false;

			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOffVURP_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				if(_isOn != _isOnDisabled){
					if(_isOn){
						_turnOff();
					}
					else{
						_turnOn();
					}
				}
				clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected
				stDisabled_In();
				_validDisablePend = false;
				_isEnabled = false;
				_outputsChange = true;
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				if(_isOn){
					_turnOff();
				}
				_isEnabled = true;
				_validEnablePend = false;
				_outputsChange = true;
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
				stDisabled_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

	default:
		break;
	}

	return;
}

void LtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
    LtchMPBttn* mpbObj = (LtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

 	if(mpbObj->getIsEnabled()){
		// Input/Output signals update
		mpbObj->updIsPressed();
		// Flags/Triggers calculation & update
		mpbObj->updValidPressesStatus();
		mpbObj->updValidUnlatchStatus();
 	}
	// State machine state update
	mpbObj->updFdaState();
	//Outputs update based on outputsChange flag
	if (mpbObj->getOutputsChange()){
		if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
		mpbObj->setOutputsChange(false);
	}

	return;
}

//=========================================================================> Class methods delimiter

TgglLtchMPBttn::TgglLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

TgglLtchMPBttn::TgglLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:TgglLtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

void TgglLtchMPBttn::stOffNVURP_Do(){
	if(_validDisablePend){
		if(_validUnlatchRlsPend)
			_validUnlatchRlsPend = false;
		_mpbFdaState = stDisabled;
		setSttChng();	//Set flag to execute exiting OUT code
	}

	return;
}

void TgglLtchMPBttn::updValidUnlatchStatus(){
	if(_isLatched){
		if(_validPressPend){
			_validUnlatchPend = true;
			_validPressPend = false;
		}
		if(_validReleasePend){
			_validUnlatchRlsPend = true;
			_validReleasePend = false;
		}
	}
}

//=========================================================================> Class methods delimiter

TmLtchMPBttn::TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _srvcTime{srvcTime}
{
	if(_srvcTime < _MinSrvcTime) //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
		_srvcTime = _MinSrvcTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
}

TmLtchMPBttn::TmLtchMPBttn(gpioPinId_t mpbttnPinStrct, const unsigned long int &srvcTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:TmLtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, srvcTime)
{
}

void TmLtchMPBttn::clrStatus(bool clrIsOn){
	_srvcTimerStrt = 0;
	LtchMPBttn::clrStatus(clrIsOn);

	return;
}

const unsigned long int TmLtchMPBttn::getSrvcTime() const{

	return _srvcTime;
}

bool TmLtchMPBttn::setSrvcTime(const unsigned long int &newSrvcTime){
	bool result {false};

   if (_srvcTime != newSrvcTime){
		if (newSrvcTime > _MinSrvcTime){  //The minimum activation time is _minActTime milliseconds
			_srvcTime = newSrvcTime;
			result = true;
		}
   }

   return result;
}

bool TmLtchMPBttn::setTmerRstbl(const bool &newIsRstbl){
    if(_tmRstbl != newIsRstbl)
        _tmRstbl = newIsRstbl;

    return _tmRstbl;
}

void TmLtchMPBttn::stOffNotVPP_Out(){
	_srvcTimerStrt = 0;

	return;
}

void TmLtchMPBttn::stOffVPP_Out(){
	_srvcTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;

	return;
}

void TmLtchMPBttn::stOffVURP_Out(){	//Check for redundant code, not needed, test!! Gaby
	_srvcTimerStrt = 0;

	return;
}

void TmLtchMPBttn::updValidUnlatchStatus(){
	if(_isLatched){
		if(_validPressPend){
			if(_tmRstbl)
				_srvcTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;
			_validPressPend = false;
		}
		if (((xTaskGetTickCount() / portTICK_RATE_MS) - _srvcTimerStrt) >= _srvcTime){
			_validUnlatchPend = true;
			_validUnlatchRlsPend = true;
		}
	}

	return;
}

//=========================================================================> Class methods delimiter

HntdTmLtchMPBttn::HntdTmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const unsigned int &wrnngPrctg, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:TmLtchMPBttn(mpbttnPort, mpbttnPin, srvcTime, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _wrnngPrctg{wrnngPrctg <= 100?wrnngPrctg:100}
{
	_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;
}

HntdTmLtchMPBttn::HntdTmLtchMPBttn(gpioPinId_t mpbttnPinStrct, const unsigned long int &srvcTime, const unsigned int &wrnngPrctg, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:HntdTmLtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, srvcTime, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, wrnngPrctg)
{
}

bool HntdTmLtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
		if (!_mpbPollTmrHndl){
			_mpbPollTmrHndl = xTimerCreate(
					_mpbPollTmrName.c_str(),  //Timer name
				pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks
				pdTRUE,     //Auto-reload true
				this,       //TimerID: data passed to the callback function to work
				mpbPollCallback
				);
			if (_mpbPollTmrHndl != NULL){
				tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
				if (tmrModResult == pdPASS)
					result = true;
			}
		}
   }

   return result;
}

void HntdTmLtchMPBttn::clrStatus(bool clrIsOn){
//	Put here class specific sets/resets, including pilot and warning
	TmLtchMPBttn::clrStatus(clrIsOn);
	_validWrnngSetPend = false;
	_validWrnngResetPend = false;
	_validPilotSetPend = false;
	_validPilotResetPend = false;

	return;
}

const bool HntdTmLtchMPBttn::getPilotOn() const{

    return _pilotOn;
}

const bool HntdTmLtchMPBttn::getWrnngOn() const{

    return _wrnngOn;
}

void HntdTmLtchMPBttn::stOffNotVPP_In(){
	if(_keepPilot){
		if(!_pilotOn){
			_pilotOn = true;
			_outputsChange = true;
		}
	}
	if(_wrnngOn){
		_wrnngOn = false;
		_outputsChange = true;
	}

	return;
}

void HntdTmLtchMPBttn::stOffVPP_Out(){
	TmLtchMPBttn::stOffVPP_Out();
	if(_pilotOn){
		_pilotOn = false;
		_outputsChange = true;
	}

	return;
}

void HntdTmLtchMPBttn::stOnNVRP_Do(){
	if(_validWrnngSetPend){
		_wrnngOn = true;
		_validWrnngSetPend = false;
		_outputsChange = true;
	}
	if(_validWrnngResetPend){
		_wrnngOn = false;
		_validWrnngResetPend = false;
		_outputsChange = true;
	}

	return;
}

void HntdTmLtchMPBttn::stLtchNVUP_Do(){
	if(_validWrnngSetPend){
		_wrnngOn = true;
		_validWrnngSetPend = false;
		_outputsChange = true;
	}
	if(_validWrnngResetPend){
		_wrnngOn = false;
		_validWrnngResetPend = false;
		_outputsChange = true;
	}

	return;
}

void HntdTmLtchMPBttn::stDisabled_In(){

	if(_validWrnngSetPend)
		_validWrnngSetPend = false;
	if(_validWrnngResetPend)
		_validWrnngResetPend = false;
	if(_wrnngOn){
		_wrnngOn = false;
		_outputsChange = true;
	}

	if(_validPilotSetPend)
		_validPilotSetPend = false;
	if(_validPilotResetPend)
		_validPilotResetPend = false;
	if(_keepPilot && !_isOnDisabled && !_pilotOn){
		_pilotOn = true;
		_outputsChange = true;
	}
	else if(_pilotOn == true)
		_pilotOn = false;

	return;
}

void HntdTmLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	HntdTmLtchMPBttn* mpbObj = (HntdTmLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	if(mpbObj->getIsEnabled()){
		// Input/Output signals update
		mpbObj->updIsPressed();
		// Flags/Triggers calculation & update
		mpbObj->updValidPressesStatus();
		mpbObj->updValidUnlatchStatus();
		mpbObj->updWrnngOn();
		mpbObj->updPilotOn();
	}
 	// State machine state update
 	mpbObj->updFdaState();

	if (mpbObj->getOutputsChange()){
		if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
		mpbObj->setOutputsChange(false);
	}

	return;
}

bool HntdTmLtchMPBttn::setKeepPilot(const bool &newKeepPilot){
	if(_keepPilot != newKeepPilot)
		_keepPilot = newKeepPilot;

	return _keepPilot;
}

bool HntdTmLtchMPBttn::setSrvcTime(const unsigned long int &newSrvcTime){
	bool result {true};

	if (newSrvcTime != _srvcTime){
		result = TmLtchMPBttn::setSrvcTime(newSrvcTime);
		if (result)
			_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;  //If the _srvcTime was changed, the _wrnngMs must be updated as it's a percentage of the first
	}

	return result;
}

bool HntdTmLtchMPBttn::setWrnngPrctg (const unsigned int &newWrnngPrctg){
	bool result{false};

	if(_wrnngPrctg != newWrnngPrctg){
		if(newWrnngPrctg <= 100){
			_wrnngPrctg = newWrnngPrctg;
			_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;
			result = true;
		}
	}

	return result;
}

bool HntdTmLtchMPBttn::updPilotOn(){
	if (_keepPilot){
		if(!_isOn && !_pilotOn){
			_validPilotSetPend = true;
			_validPilotResetPend = false;
		}
		else if(_isOn && _pilotOn){
			_validPilotResetPend = true;
			_validPilotSetPend = false;
		}
	}
	else{
		if(_pilotOn){
			_validPilotResetPend = true;
			_validPilotSetPend = false;
		}
	}

	return _pilotOn;
}

bool HntdTmLtchMPBttn::updWrnngOn(){
//Disengaging the _wrnngOn direct signal manipulation, replacing it with a flag mechanisms
//	that relays the turning on or off in the fda
	if(_wrnngPrctg > 0){
		if (_isOn && _isEnabled){	//The _isEnabled evaluation is done to avoid computation of flags that will be ignored if the MPB is disablee
			if (((xTaskGetTickCount() / portTICK_RATE_MS) - _srvcTimerStrt) >= (_srvcTime - _wrnngMs)){
				if(_wrnngOn == false){
					_validWrnngSetPend = true;
					_validWrnngResetPend = false;
				}
			}
			else if(_wrnngOn == true){
				_validWrnngResetPend = true;
				_validWrnngSetPend = false;
			}
		}
		else if(_wrnngOn == true){
			_validWrnngResetPend = true;
			_validWrnngSetPend = false;
		}
	}

	return _wrnngOn;
}

//=========================================================================> Class methods delimiter

XtrnUnltchMPBttn::XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _unLtchBttn{unLtchBttn}
{
}

XtrnUnltchMPBttn::XtrnUnltchMPBttn(gpioPinId_t mpbttnPinStrct, DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:XtrnUnltchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, unLtchBttn, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

XtrnUnltchMPBttn::XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

XtrnUnltchMPBttn::XtrnUnltchMPBttn(gpioPinId_t mpbttnPinStrct,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:XtrnUnltchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool XtrnUnltchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
		if (!_mpbPollTmrHndl){
			_mpbPollTmrHndl = xTimerCreate(
					_mpbPollTmrName.c_str(),  //Timer name
				pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks
				pdTRUE,     //Auto-reload true
				this,       //TimerID: data passed to the callback function to work
				mpbPollCallback
			);
		}
		if (_mpbPollTmrHndl != NULL){
			tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
			if (tmrModResult == pdPASS){
				if(_unLtchBttn != nullptr)
					result = _unLtchBttn->begin();
				else
					result = true;
			}
		}
   }

   return result;
}

void XtrnUnltchMPBttn::clrStatus(bool clrIsOn){
	_xtrnUnltchPRlsCcl = false;
	LtchMPBttn::clrStatus(clrIsOn);

	return;
}

void XtrnUnltchMPBttn::stOffNVURP_Do(){
	if(_validDisablePend){
		if(_validUnlatchRlsPend)
			_validUnlatchRlsPend = false;
		if(_xtrnUnltchPRlsCcl)
			_xtrnUnltchPRlsCcl = false;
		_mpbFdaState = stDisabled;
		setSttChng();	//Set flag to execute exiting OUT code
	}

	return;
}

void XtrnUnltchMPBttn::updValidUnlatchStatus(){

	if(_unLtchBttn != nullptr){
		if(_isLatched){
			if (_unLtchBttn->getIsOn() && !_xtrnUnltchPRlsCcl){
				_validUnlatchPend = true;
				_xtrnUnltchPRlsCcl = true;
			}
			if(!_unLtchBttn->getIsOn() && _xtrnUnltchPRlsCcl){
				_validUnlatchRlsPend = true;
				_xtrnUnltchPRlsCcl = false;
			}
		}
		else{
			if(_xtrnUnltchPRlsCcl)
				_xtrnUnltchPRlsCcl = false;
		}
	}
	return;
}

//=========================================================================> Class methods delimiter

DblActnLtchMPBttn::DblActnLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DblActnLtchMPBttn::DblActnLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DblActnLtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DblActnLtchMPBttn::~DblActnLtchMPBttn()
{
}

bool DblActnLtchMPBttn::begin(const unsigned long int &pollDelayMs) {
    bool result {false};
    BaseType_t tmrModResult {pdFAIL};

    if (pollDelayMs > 0){
        if (!_mpbPollTmrHndl){
            _mpbPollTmrHndl = xTimerCreate(
            		_mpbPollTmrName.c_str(),  //Timer name
                pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks
                pdTRUE,     //Auto-reload true
                this,       //TimerID: data passed to the callback function to work
                mpbPollCallback	  //Callback function
				);
            if (_mpbPollTmrHndl != NULL){
               tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
            	if (tmrModResult == pdPASS)
                  result = true;
            }
        }
    }

    return result;
}

void DblActnLtchMPBttn::clrStatus(bool clrIsOn){
	_scndModTmrStrt = 0;
	_validScndModPend = false;
	LtchMPBttn::clrStatus(clrIsOn);

	return;
}

unsigned long DblActnLtchMPBttn::getScndModActvDly(){

	return _scndModActvDly;
}

bool DblActnLtchMPBttn::setScndModActvDly(const unsigned long &newVal){
	bool result{false};

	if(newVal != _scndModActvDly){
		_scndModActvDly = newVal;
		result = true;
	}

	return result;
}

void DblActnLtchMPBttn::stOnStrtScndMod_In(){

	return;
}

void DblActnLtchMPBttn::stOnEndScndMod_Out(){

	return;
}

void DblActnLtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend || _validScndModPend){
				_mpbFdaState = stOffVPP;	//Start pressing timer
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_isOn){
				_turnOn();
			}
			if(_validScndModPend){
				_scndModTmrStrt = (xTaskGetTickCount() / portTICK_RATE_MS);
				_mpbFdaState = stOnStrtScndMod;
				setSttChng();
			}
			else if(_validPressPend && _validReleasePend){
				_validPressPend = false;
				_validReleasePend = false;
				_mpbFdaState = stOnMPBRlsd;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnStrtScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){
				stOnStrtScndMod_In();
				clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOnScndMod;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_validReleasePend){
				//Operating in Second Mode
				stOnScndMod_Do();
			}
			else{
				// MPB released, close Slider mode, move on to next state
				_mpbFdaState = stOnEndScndMod;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnEndScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_scndModTmrStrt = 0;
			_validScndModPend = false;
			_mpbFdaState = stOnMPBRlsd;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOnEndScndMod_Out();
			}
			break;

		case stOnMPBRlsd:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validScndModPend){
				_scndModTmrStrt = (xTaskGetTickCount() / portTICK_RATE_MS);
				_mpbFdaState = stOnStrtScndMod;
				setSttChng();
			}
			else if(_validPressPend && _validReleasePend){
				_validPressPend = false;
				_validReleasePend = false;
				_mpbFdaState = stOnTurnOff;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnTurnOff:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOff();
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				if(_isOn != _isOnDisabled){
					if(_isOn){
						_turnOff();
					}
					else{
						_turnOn();
					}
				}
				clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected
				stDisabled_In();
				_isEnabled = false;
				_validDisablePend = false;
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				_isEnabled = true;
				_validEnablePend = false;
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}

			//Out: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
			}	// Execute this code only ONCE, when exiting this state
			break;
	default:
		break;
	}

	return;
}

bool DblActnLtchMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(_dbncRlsTimerStrt != 0)
			_dbncRlsTimerStrt = 0;
		if(_dbncTimerStrt == 0){    //It was not previously pressed
			_dbncTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be pressed
		}
		else{
			if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= ((_dbncTimeTempSett + _strtDelay) + _scndModActvDly)){
				_validScndModPend = true;
				_validPressPend = false;
			} else if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
				_validPressPend = true;
			}
			if(_validPressPend || _validScndModPend){
				_validReleasePend = false;
				_prssRlsCcl = true;
			}
		}
	}
	else{
		if(_dbncTimerStrt != 0)
			_dbncTimerStrt = 0;
		if(!_validReleasePend && _prssRlsCcl){
			if(_dbncRlsTimerStrt == 0){    //It was not previously pressed
				_dbncRlsTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be UNpressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
					_validReleasePend = true;
					_prssRlsCcl = false;
				}
			}
		}
	}

	return (_validPressPend || _validScndModPend);
}

void DblActnLtchMPBttn::updValidUnlatchStatus(){
	_validUnlatchPend = true;

	return;
}

void DblActnLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	DblActnLtchMPBttn* mpbObj = (DblActnLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	if(mpbObj->getIsEnabled()){
		// Input/Output signals update
		mpbObj->updIsPressed();
		// Flags/Triggers calculation & update
		mpbObj->updValidPressesStatus();
	}
 	// State machine state update
	mpbObj->updFdaState();

	if (mpbObj->getOutputsChange()){
	  if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
	  mpbObj->setOutputsChange(false);
	}

	return;
}

//=========================================================================> Class methods delimiter

DDlydDALtchMPBttn::DDlydDALtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DblActnLtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DDlydDALtchMPBttn::DDlydDALtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DDlydDALtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DDlydDALtchMPBttn::~DDlydDALtchMPBttn()
{
}

void DDlydDALtchMPBttn::clrStatus(bool clrIsOn){
	if(clrIsOn && _isOn2){
		_isOn2= false;
		_outputsChange = true;
	}
	DblActnLtchMPBttn::clrStatus(clrIsOn);

	return;
}

bool DDlydDALtchMPBttn::getIsOn2(){

	return _isOn2;
}

void DDlydDALtchMPBttn::stDisabled_In(){
	if(_isOn2 != _isOnDisabled){
		_isOn2 = _isOnDisabled;
		_outputsChange = true;
	}

	return;
}

void DDlydDALtchMPBttn::stOnStrtScndMod_In(){
	if(!_isOn2){
		_isOn2 = true;
		_outputsChange = true;
	}

	return;
}

void DDlydDALtchMPBttn::stOnScndMod_Do(){

	return;
}

void DDlydDALtchMPBttn::stOnEndScndMod_Out(){
	if(_isOn2){
		_isOn2 = false;
		_outputsChange = true;
	}

	return;
}

//=========================================================================> Class methods delimiter

SldrDALtchMPBttn::SldrDALtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long &dbncTimeOrigSett, const unsigned long int &strtDelay, const uint16_t initVal)
:DblActnLtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _otptCurVal{initVal}
{
	if(_otptCurVal < _otptValMin || _otptCurVal > _otptValMax)
		_otptCurVal = _otptValMin;	// Original development setup makes this outside limits situation impossible, as the limits are set to the full range of the data type used
}

SldrDALtchMPBttn::SldrDALtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long &dbncTimeOrigSett, const unsigned long int &strtDelay, const uint16_t initVal)
:SldrDALtchMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, initVal)
{
}

SldrDALtchMPBttn::~SldrDALtchMPBttn()
{
}

void SldrDALtchMPBttn::clrStatus(bool clrIsOn){
	DblActnLtchMPBttn::clrStatus(clrIsOn);

	return;
}

uint16_t SldrDALtchMPBttn::getOtptCurVal(){

	return _otptCurVal;
}

bool SldrDALtchMPBttn::getOtptCurValIsMax(){

	return (_otptCurVal == _otptValMax);
}

bool SldrDALtchMPBttn::getOtptCurValIsMin(){

	return (_otptCurVal == _otptValMin);
}

unsigned long SldrDALtchMPBttn::getOtptSldrSpd(){

	return _otptSldrSpd;
}

uint16_t SldrDALtchMPBttn::getOtptSldrStpSize(){

	return _otptSldrStpSize;
}

uint16_t SldrDALtchMPBttn::getOtptValMax(){

	return _otptValMax;
}

uint16_t SldrDALtchMPBttn::getOtptValMin(){

	return _otptValMin;
}

bool SldrDALtchMPBttn::getSldrDirUp(){

	return _curSldrDirUp;
}

bool SldrDALtchMPBttn::setOtptCurVal(const uint16_t &newVal){
	bool result{false};

	if(_otptCurVal != newVal){
		if(newVal >= _otptValMin && newVal <= _otptValMax){
			_otptCurVal = newVal;
			result = true;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptSldrSpd(const uint16_t &newVal){
	bool result{false};

	if(newVal != _otptSldrSpd){
		if(newVal > 0){	//The "Change Value Speed" must be a positive number representing the time in milliseconds between "Output Values" change the size of a "Value Step Quantity" while the MPB is being kept pressed
			_otptSldrSpd = newVal;
			result = true;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptSldrStpSize(const uint16_t &newVal){
	bool result{false};

	if(newVal != _otptSldrStpSize){
		if(newVal <= (_otptValMax - _otptValMin)){	//If newVal == (_otptValMax - _otptValMin) the slider will work as kind of an On/Off switch
			_otptSldrStpSize = newVal;
			result = true;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptValMax(const uint16_t &newVal){
	bool result{false};

	if(newVal != _otptValMax){
		if(newVal > _otptValMin){
			_otptValMax = newVal;
			if(_otptCurVal > _otptValMax){
				_otptCurVal = _otptValMax;
				_outputsChange = true;
			}
			result = true;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptValMin(const uint16_t &newVal){
	bool result{false};

	if(newVal != _otptValMin){
		if(newVal < _otptValMax){
			_otptValMin = newVal;
			if(_otptCurVal < _otptValMin){
				_otptCurVal = _otptValMin;
				_outputsChange = true;
			}
			result = true;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::_setSldrDir(const bool &newVal){
	bool result{false};

	if(newVal != _curSldrDirUp){
		if(newVal){	//Try to set new direction Up
			if(_otptCurVal != _otptValMax){
				_curSldrDirUp = true;
				result = true;
			}
		}
		else{		//Try to set new direction down
			if(_otptCurVal != _otptValMin){
				_curSldrDirUp = false;
				result = true;
			}
		}
	}

	return result;
}

bool SldrDALtchMPBttn::setSldrDirDn(){

	return _setSldrDir(false);
}

bool SldrDALtchMPBttn::setSldrDirUp(){

	return _setSldrDir(true);
}

bool SldrDALtchMPBttn::setSwpDirOnEnd(const bool &newVal){

	if(_autoSwpDirOnEnd != newVal)
		_autoSwpDirOnEnd = newVal;

	return _autoSwpDirOnEnd;
}

bool SldrDALtchMPBttn::setSwpDirOnPrss(const bool &newVal){

	if(_autoSwpDirOnPrss != newVal)
		_autoSwpDirOnPrss = newVal;

	return _autoSwpDirOnEnd;
}

void SldrDALtchMPBttn::stOnScndMod_Do(){
	// Operating in Slider mode, change the associated value according to the time elapsed since last update
	//and the step size for every time unit elapsed
	uint16_t _otpStpsChng{0};
	unsigned long _sldrTmrNxtStrt{0};
	unsigned long _sldrTmrRemains{0};

	_sldrTmrNxtStrt = (xTaskGetTickCount() / portTICK_RATE_MS);
	_otpStpsChng = (_sldrTmrNxtStrt - _scndModTmrStrt) /_otptSldrSpd;
	_sldrTmrRemains = ((_sldrTmrNxtStrt - _scndModTmrStrt) % _otptSldrSpd) * _otptSldrSpd;
	_sldrTmrNxtStrt -= _sldrTmrRemains;
	_scndModTmrStrt = _sldrTmrNxtStrt;	//This ends the time management section of the state, calculating the time

	if(_curSldrDirUp){
		// The slider is moving up
		if(_otptCurVal != _otptValMax){
			if((_otptValMax - _otptCurVal) >= (_otpStpsChng * _otptSldrStpSize)){
				//The value change is in range
				_otptCurVal += (_otpStpsChng * _otptSldrStpSize);
			}
			else{
				//The value change goes out of range
				_otptCurVal = _otptValMax;
			}
			_outputsChange = true;
		}
		if(_outputsChange){
			if(_otptCurVal == _otptValMax){
				if(_autoSwpDirOnEnd == true){
					_curSldrDirUp = false;
				}
			}
		}
	}
	else{
		// The slider is moving down
		if(_otptCurVal != _otptValMin){
			if((_otptCurVal - _otptValMin) >= (_otpStpsChng * _otptSldrStpSize)){
				//The value change is in range
				_otptCurVal -= (_otpStpsChng * _otptSldrStpSize);
			}
			else{
				//The value change goes out of range
				_otptCurVal = _otptValMin;
			}
			_outputsChange = true;
		}
		if(_outputsChange){
			if(_otptCurVal == _otptValMin){
				if(_autoSwpDirOnEnd == true){
					_curSldrDirUp = true;
				}
			}
		}
	}

	return;
}

void SldrDALtchMPBttn::stOnStrtScndMod_In(){
	if(_autoSwpDirOnPrss)
		swapSldrDir();

	return;
}

bool SldrDALtchMPBttn::swapSldrDir(){

	return _setSldrDir(!_curSldrDirUp);
}

//=========================================================================> Class methods delimiter

VdblMPBttn::VdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:DbncdDlydMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
	_isOnDisabled = isOnDisabled;
}

VdblMPBttn::VdblMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:VdblMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

VdblMPBttn::~VdblMPBttn()
{
}

void VdblMPBttn::clrStatus(bool clrIsOn){
	DbncdMPBttn::clrStatus(clrIsOn);	//This method might set the _outputsChange flag if the _isOn flag was set, as it will be changed to false if the parameter clrIsOn == true.
	setIsNotVoided();

	return;
}

const bool VdblMPBttn::getIsVoided() const{

    return _isVoided;
}

void VdblMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	VdblMPBttn* mpbObj = (VdblMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	if(mpbObj->getIsEnabled()){
		// Input/Output signals update
		mpbObj->updIsPressed();
		// Flags/Triggers calculation & update
		mpbObj->updValidPressesStatus();
		mpbObj->updVoidStatus();
	}
 	// State machine state update
	mpbObj->updFdaState();

	if (mpbObj->getOutputsChange()){
	  if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
	  mpbObj->setOutputsChange(false);
	}

	return;
}

bool VdblMPBttn::setIsNotVoided(){

	return setVoided(false);
}

bool VdblMPBttn::setIsVoided(){

	return setVoided(true);
}

bool VdblMPBttn::setVoided(const bool &newVoidValue){
	if(_isVoided != newVoidValue){
		_isVoided = newVoidValue;
      _outputsChange = true;
	}

	return _isVoided;
}

void VdblMPBttn::stDisabled_In(){
	pause();    //It's pausing the timer that keeps the inputs updated and calculates and updates the output flags... Flags must be updated for the disabled condition
	clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected

	if(_isOn != _isOnDisabled){
		_isOn = _isOnDisabled;
		if(_isOn){
			_turnOff();
		}
		else{
			_turnOn();
		}
	}

	return;
}

void VdblMPBttn::stDisabled_Out(){
	clrStatus(true);	//Clears all flags and timers, _isOn value will be reset

	return;
}

void VdblMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				_isVoided = false;
				stOffNotVPP_In();
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_isOn){
				_turnOn();
			}
			_validPressPend = false;
			stOffVPP_Do();	// This provides a setting point for the voiding mechanism to be started
			_mpbFdaState = stOnNVRP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnNVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validVoidPend){
				_mpbFdaState = stOnVVP;
				setSttChng();
			}
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVVP:
			if(_sttChng){
				_isVoided = true;
				_validVoidPend = false;
				_outputsChange = true;
				clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOnVddNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVddNVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOff();
			_mpbFdaState = stOffVddNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVddNVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			stOffVddNVUP_Do();
			if(_validUnvoidPend){
				_mpbFdaState = stOffVddVUP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVddVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_isVoided = false;
			_validUnvoidPend = false;
			_mpbFdaState = stOffUnVdd;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffUnVdd:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOff;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validReleasePend = false;
			_mpbFdaState = stOnTurnOff;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnTurnOff:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOff();
			_mpbFdaState = stOff;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOff:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				_validDisablePend = false;
				if(_isOn != _isOnDisabled){
					if(_isOn){
						_turnOff();
					}
					else{
						_turnOn();
					}
				}
				clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected
				_isEnabled = false;
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				_turnOff();
				_isEnabled = true;
				_validEnablePend = false;
				_outputsChange = true;
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				stDisabled_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

	default:
		break;
	}

	return;
}

//=========================================================================> Class methods delimiter

TmVdblMPBttn::TmVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:VdblMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, isOnDisabled), _voidTime{voidTime}
{
}

TmVdblMPBttn::TmVdblMPBttn(gpioPinId_t mpbttnPinStrct, unsigned long int voidTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:TmVdblMPBttn(mpbttnPinStrct.portId, mpbttnPinStrct.pinNum, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, isOnDisabled, voidTime)
{
}

TmVdblMPBttn::~TmVdblMPBttn()
{
}

bool TmVdblMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (!_mpbPollTmrHndl){
		_mpbPollTmrHndl = xTimerCreate(
				_mpbPollTmrName.c_str(),  //Timer name
			pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks
			pdTRUE,     //Autoreload true
			this,       //TimerID: data passed to the callback funtion to work
			mpbPollCallback
		);
	}
   if (_mpbPollTmrHndl != NULL){
   	tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
		if (tmrModResult == pdPASS)
			result = true;
	}

return result;
}

void TmVdblMPBttn::clrStatus(){
    /*
     To Resume operation after a pause() without risking generating false "Valid presses" and "On" situations,
    several attributes must be reseted to "Start" values
    */

    VdblMPBttn::clrStatus();
    _voidTmrStrt = 0;

    return;
}

const unsigned long int TmVdblMPBttn::getVoidTime() const{

    return _voidTime;
}

bool TmVdblMPBttn::setVoidTime(const unsigned long int &newVoidTime){
    bool result{false};

    if((newVoidTime != _voidTime) && (newVoidTime > _MinSrvcTime)){
        _voidTime = newVoidTime;
        result = true;
    }

    return result;
}

void TmVdblMPBttn::stOffNotVPP_In(){
	_voidTmrStrt = 0;

	return;
}

void TmVdblMPBttn::stOffVPP_Do(){	// This provides a setting point for the voiding mechanism to be started
   _voidTmrStrt = xTaskGetTickCount() / portTICK_RATE_MS;

	return;
}

void TmVdblMPBttn::stOffVddNVUP_Do(){
	if(_validReleasePend){
		_validReleasePend = false;
		_validUnvoidPend = true;
	}
	return;
}

bool TmVdblMPBttn::updIsPressed(){

    return DbncdDlydMPBttn::updIsPressed();
}

bool TmVdblMPBttn::updVoidStatus(){
   bool result {false};

   if(_voidTmrStrt != 0){
		if (((xTaskGetTickCount() / portTICK_RATE_MS) - _voidTmrStrt) >= (_voidTime)){ // + _dbncTimeTempSett + _strtDelay
			 result = true;
		}
	}
   _validVoidPend = result;

	return _validVoidPend;
}

//=========================================================================> Class methods delimiter

/**
 * @brief Returns the position of the single set bit on an unsigned 16 bits integer value
 *
 * @param mask A 16 bits single bit set unsigned integer
 * @return The position of the single bit set on the parameter value
 * - An unsigned 8 bits integer in the range 0 ~ 15 if the parameter is a single bit set value indicating the position of that set bit.
 * - 0xFF otherwise.
 */
uint8_t singleBitPosNum(uint16_t mask){
	uint8_t result{0xFF};

	if((mask > 0) && (mask & (mask - 1)) == 0){
		result = 0;
		mask = mask >> 1;
		while (mask > 0){
			mask = mask >> 1;
			++result;
		}
	}

	return result;
}

