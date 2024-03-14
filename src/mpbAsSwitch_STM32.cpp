/*
 * @file		: mpbAsSwitch_STM32.cpp
 * @brief	: Header file for
 *
 * @author	: Gabriel D. Goldman
 * @date		: Created on: Nov 6, 2023
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
		char mpbttnPinChar[3]{};
		int tmpBitCount{0};
		uint16_t tmpPinNum {_mpbttnPin};

		tmpPinNum = tmpPinNum >> 1;
		while(tmpPinNum > 0){
			tmpPinNum = tmpPinNum >> 1;
			++tmpBitCount;
		}
		sprintf(mpbttnPinChar, "%02u", tmpBitCount);
		strcpy(_mpbPollTmrName, "PollMpbPin");
		if(mpbttnPort == GPIOA){
			strcat(_mpbPollTmrName, "A");
			__HAL_RCC_GPIOA_CLK_ENABLE();	//Sets the bit in the GPIO enabled clocks register, by logic OR of the corresponding bit, no problem if already set, macro adds time to get the clk running
		}
		else if(mpbttnPort == GPIOB){
			strcat(_mpbPollTmrName, "B");
			__HAL_RCC_GPIOB_CLK_ENABLE();
		}
		else if(mpbttnPort == GPIOC){
			strcat(_mpbPollTmrName, "C");
			__HAL_RCC_GPIOC_CLK_ENABLE();
		}
		else if(mpbttnPort == GPIOD){
			strcat(_mpbPollTmrName, "D");
			__HAL_RCC_GPIOD_CLK_ENABLE();
		}
#ifdef GPIOE
		else if(mpbttnPort == GPIOE){
			strcat(_mpbPollTmrName, "E");
			__HAL_RCC_GPIOE_CLK_ENABLE();
		}
#endif
#ifdef GPIOF
		else if(mpbttnPort == GPIOF){	//Port not present in all STM32 MCUs/DevBoards
			strcat(_mpbPollTmrName, "F");
			__HAL_RCC_GPIOF_CLK_ENABLE();
		}
#endif
#ifdef GPIOG
		else if(mpbttnPort == GPIOG){	//Port not present in all STM32 MCUs/DevBoards
			strcat(_mpbPollTmrName, "G");
			__HAL_RCC_GPIOG_CLK_ENABLE();
		}
#endif
#ifdef GPIOH
		else if(mpbttnPort == GPIOH){	//Port not present in all STM32 MCUs/DevBoards
			strcat(_mpbPollTmrName, "H");
			__HAL_RCC_GPIOH_CLK_ENABLE();
		}
#endif
#ifdef GPIOI
		else if(mpbttnPort == GPIOI){	//Port not present in all STM32 MCUs/DevBoards
			strcat(_mpbPollTmrName, "I");
			__HAL_RCC_GPIOI_CLK_ENABLE();
}
#endif
		strcat(_mpbPollTmrName, mpbttnPinChar);
		strcat(_mpbPollTmrName, "_tmr");

		if(_dbncTimeOrigSett < _stdMinDbncTime) 	// Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
			_dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
		_dbncTimeTempSett = _dbncTimeOrigSett;
		_dbncRlsTimeTempSett = _dbncTimeOrigSett;

		/*Configure GPIO pin : _mpbttnPin */
      HAL_GPIO_WritePin(_mpbttnPort, _mpbttnPin, GPIO_PIN_RESET);

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
                _mpbPollTmrName,  //Timer name
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

void DbncdMPBttn::clrStatus(){
	/*To Resume operations after a pause() without risking generating false "Valid presses" and "On" situations,
	several attributes must be reseted to "Start" values*/
	_isPressed = false;
	_validPressPend = false;
	_validReleasePend = false;
	_dbncTimerStrt = 0;
	_dbncRlsTimerStrt = 0;
	if(_isOn){
		_isOn = false;
		_outputsChange = true;
	}

	return;
}

void DbncdMPBttn::clrSttChng(){
	_sttChng = false;

	return;
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

const bool DbncdMPBttn::getIsOn() const {

	return _isOn;
}

const bool DbncdMPBttn::getIsPressed() const {

	return _isPressed;
}

const bool DbncdMPBttn::getOutputsChange() const{

    return _outputsChange;
}

const TaskHandle_t DbncdMPBttn::getTaskToNotify() const{

    return _taskToNotifyHndl;
}

bool DbncdMPBttn::init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett){
    char mpbttnPinChar[3]{};
    int tmpBitCount{0};
    uint16_t tmpPinNum {_mpbttnPin};
    bool result {false};

    if (_mpbPollTmrName[0] == '\0'){
        _mpbttnPin = mpbttnPin;
        _pulledUp = pulledUp;
        _typeNO = typeNO;
        _dbncTimeOrigSett = dbncTimeOrigSett;

        tmpPinNum = tmpPinNum >> 1;
        while(tmpPinNum > 0){
       	 tmpPinNum = tmpPinNum >> 1;
       	 ++tmpBitCount;
        }
        sprintf(mpbttnPinChar, "%02u", tmpBitCount);
        strcpy(_mpbPollTmrName, "PollMpbPin");
  		if(mpbttnPort == GPIOA){
  			strcat(_mpbPollTmrName, "A");
  			__HAL_RCC_GPIOA_CLK_ENABLE();	//Sets the bit in the GPIO enabled clocks register, by logic OR of the corresponding bit, no problem if already set, macro adds time to get the clk running
  		}
  		else if(mpbttnPort == GPIOB){
  			strcat(_mpbPollTmrName, "B");
  			__HAL_RCC_GPIOB_CLK_ENABLE();
  		}
  		else if(mpbttnPort == GPIOC){
  			strcat(_mpbPollTmrName, "C");
  			__HAL_RCC_GPIOC_CLK_ENABLE();
  		}
  		else if(mpbttnPort == GPIOD){
  			strcat(_mpbPollTmrName, "D");
  			__HAL_RCC_GPIOD_CLK_ENABLE();
  		}
  		else if(mpbttnPort == GPIOE){
  			strcat(_mpbPollTmrName, "E");
  			__HAL_RCC_GPIOE_CLK_ENABLE();
  		}
  #ifdef GPIOF
  		else if(mpbttnPort == GPIOF){	//Port not present in all STM32 MCUs/DevBoards
  			strcat(_mpbPollTmrName, "F");
  			__HAL_RCC_GPIOF_CLK_ENABLE();
  		}
  #endif
  #ifdef GPIOG
  		else if(mpbttnPort == GPIOG){	//Port not present in all STM32 MCUs/DevBoards
  			strcat(_mpbPollTmrName, "G");
  			__HAL_RCC_GPIOG_CLK_ENABLE();
  		}
  #endif
  #ifdef GPIOH
  		else if(mpbttnPort == GPIOH){	//Port not present in all STM32 MCUs/DevBoards
  			strcat(_mpbPollTmrName, "H");
  			__HAL_RCC_GPIOH_CLK_ENABLE();
  		}
  #endif
  #ifdef GPIOI
  		else if(mpbttnPort == GPIOI){	//Port not present in all STM32 MCUs/DevBoards
  			strcat(_mpbPollTmrName, "I");
  			__HAL_RCC_GPIOI_CLK_ENABLE();
  }
  #endif

        if(_dbncTimeOrigSett < _stdMinDbncTime) 	// Best practice would impose failing the constructor (throwing an exeption or building a "zombie" object)
            _dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
        _dbncTimeTempSett = _dbncTimeOrigSett;

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

void DbncdMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	DbncdMPBttn *mpbObj = (DbncdMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	// Input/Output signals update
	mpbObj->updIsPressed();
	// Flags/Triggers calculation & update
 	mpbObj->updValidPressesStatus();
 	// State machine status update
	mpbObj->updFdaState();

   if (mpbObj->getOutputsChange()){
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
	_mpbFdaState = stOffNotVPP;

	return true;
}

bool DbncdMPBttn::resume(){
	bool result {false};
	BaseType_t tmrModResult {pdFAIL};

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

    if (newDbncTime >= _stdMinDbncTime){
        _dbncTimeTempSett = newDbncTime;
        result = true;
    }

    return result;
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

void DbncdMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(!_isOn){
				_isOn = true;
				_outputsChange = true;
			}
			_validPressPend = false;
			_mpbFdaState = stOn;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOn:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_isOn){
				_isOn = false;
				_outputsChange = true;
			}
			_validReleasePend = false;
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}

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
		if(!_isOn){
			if(_dbncRlsTimerStrt != 0)
				_dbncRlsTimerStrt = 0;
			if(_dbncTimerStrt == 0){    //This is the first detection of the press event
				_dbncTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be pressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= (_dbncTimeTempSett)){
					_validPressPend = true;
					_validReleasePend = false;
				}
			}
		}
	}
	else{
		if(_isOn){
			if(_dbncTimerStrt != 0)
				_dbncTimerStrt = 0;
			if(_dbncRlsTimerStrt == 0){    //This is the first detection of the release event
				_dbncRlsTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be UNpressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
//					_validPressPend = false;
					_validReleasePend = true;
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
:DbncdMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett), _strtDelay{strtDelay}
{
}

bool DbncdDlydMPBttn::begin(const unsigned long int &pollDelayMs) {
	bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
       if (!_mpbPollTmrHndl){
           _mpbPollTmrHndl = xTimerCreate(
               _mpbPollTmrName,  //Timer name
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

unsigned long int DbncdDlydMPBttn::getStrtDelay(){

	return _strtDelay;
}

bool DbncdDlydMPBttn::init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay){
    bool result {false};

    result = DbncdMPBttn::init(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett);
    if (result)
        result = setStrtDelay(strtDelay);

    return result;
}

void DbncdDlydMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
    DbncdDlydMPBttn* mpbObj = (DbncdDlydMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

 	// Input/Output signals update
 	mpbObj->updIsPressed();
 	// Flags/Triggers calculation & update
  	mpbObj->updValidPressesStatus();
  	// State machine state update
 	mpbObj->updFdaState();

	if (mpbObj->getOutputsChange()){
		if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
		mpbObj->setOutputsChange(false);
	}

	return;
}

bool DbncdDlydMPBttn::setStrtDelay(const unsigned long int &newStrtDelay){
    if(_strtDelay != newStrtDelay){
        _strtDelay = newStrtDelay;
    }

    return true;
}

bool DbncdDlydMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(!_isOn){
			if(_dbncRlsTimerStrt != 0)
				_dbncRlsTimerStrt = 0;
			if(_dbncTimerStrt == 0){    //This is the first detection of the press event
				_dbncTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be pressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
					_validPressPend = true;
					_validReleasePend = false;
				}
			}
		}
	}
	else{
		if(_isOn){
			if(_dbncTimerStrt != 0)
				_dbncTimerStrt = 0;
			if(_dbncRlsTimerStrt == 0){    //This is the first detection of the release event
				_dbncRlsTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be UNpressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
//					_validPressPend = false;
					_validReleasePend = true;
				}
			}
		}
	}

	return (_validPressPend||_validReleasePend);
}

//=========================================================================> Class methods delimiter

LtchMPBttn::LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DbncdDlydMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool LtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
   	if (!_mpbPollTmrHndl){
   		_mpbPollTmrHndl = xTimerCreate(
				_mpbPollTmrName,  //Timer name
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

void LtchMPBttn::clrStatus(){
	_isLatched = false;
	_validUnlatchPend = false;
	DbncdMPBttn::clrStatus();

	return;
}

const bool LtchMPBttn::getIsLatched() const{

	return _isLatched;
}

const bool LtchMPBttn::getUnlatchPend() const{

	return _validUnlatchPend;
}

bool LtchMPBttn::setUnlatchPend(const bool &newVal){
	if(_validUnlatchPend != newVal)
		_validUnlatchPend = newVal;

	return _validUnlatchPend;
}

bool LtchMPBttn::unlatch(){
	if(_isLatched){
		_validUnlatchPend = true;
	}

	return _isLatched;
}

void LtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_isLatched = false;
				_validUnlatchPend = false;
				_mpbFdaState = stOffVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(!_isOn){
				_isOn = true;
				_outputsChange = true;
			}
			_validPressPend = false;
			_mpbFdaState = stOnNVRP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOnNVRP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_validReleasePend = false;
			if(!_isLatched)
				_isLatched = true;
			_mpbFdaState = stLtchNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stLtchNVUP:	//From this state on different unlatch sources might make sense
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validUnlatchPend){
				_mpbFdaState = stLtchdVUP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stLtchdVUP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_mpbFdaState = stOffVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVUP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_validUnlatchPend = false;	// This is a placeholder for updValidUnlatchStatus() implemented in each subclass
			_mpbFdaState = stOffNVURP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffNVURP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validReleasePend){
				_mpbFdaState = stOffVURP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVURP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_validReleasePend = false;
			if(_isOn){
				_isOn = false;
				_outputsChange = true;
			}
			if(_isLatched)
				_isLatched = false;
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

	default:
		break;
	}

	return;
}

void LtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
    LtchMPBttn* mpbObj = (LtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

  	// Input/Output signals update
  	mpbObj->updIsPressed();
  	// Flags/Triggers calculation & update
	mpbObj->updValidPressesStatus();
	mpbObj->updValidUnlatchStatus();
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
TgglLtchMPBttn::TgglLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool TgglLtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
   	if (!_mpbPollTmrHndl){
   		_mpbPollTmrHndl = xTimerCreate(
				_mpbPollTmrName,  //Timer name
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

bool TgglLtchMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(!_validPressPend){
			if(_dbncRlsTimerStrt != 0)
				_dbncRlsTimerStrt = 0;
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
		if(!_validReleasePend && _prssRlsCcl){
			if(_dbncTimerStrt != 0)
				_dbncTimerStrt = 0;
			if(_dbncRlsTimerStrt == 0){    //This is the first detection of the release event
				_dbncRlsTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be UNpressed
			}
			else{
				if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
//					_validPressPend = false;
					_validReleasePend = true;
					_prssRlsCcl = false;
				}
			}
		}
	}

	return (_validPressPend||_validReleasePend);
}

void TgglLtchMPBttn::updValidUnlatchStatus(){
	if(_isLatched){
		if(_validPressPend){
			_validUnlatchPend = true;
			_validPressPend = false;
		}
	}
}

void TgglLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
    TgglLtchMPBttn* mpbObj = (TgglLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

  	// Input/Output signals update
  	mpbObj->updIsPressed();
  	// Flags/Triggers calculation & update
	mpbObj->updValidPressesStatus();
	mpbObj->updValidUnlatchStatus();
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

TmLtchMPBttn::TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _srvcTime{srvcTime}
{
	if(_srvcTime < _MinSrvcTime) //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
		_srvcTime = _MinSrvcTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
}

bool TmLtchMPBttn::begin(const unsigned long int &pollDelayMs){
	bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
		if (!_mpbPollTmrHndl){
			_mpbPollTmrHndl = xTimerCreate(
				_mpbPollTmrName,  //Timer name
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

void TmLtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_isLatched = false;
				_validUnlatchPend = false;
				_srvcTimerStrt = 0;
				_mpbFdaState = stOffVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(!_isOn){
				_isOn = true;
				_outputsChange = true;
			}
			_validPressPend = false;
			_isLatched = true;
			_srvcTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;
			_mpbFdaState = stOnNVRP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOnNVRP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_validReleasePend = false;
			_mpbFdaState = stLtchNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stLtchNVUP:	//From this state on different unlatch sources might make sense
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validUnlatchPend){
				_mpbFdaState = stLtchdVUP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stLtchdVUP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_isOn){
				_isOn = false;
				_outputsChange = true;
			}
			_mpbFdaState = stOffVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVUP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_validUnlatchPend = false;
			_mpbFdaState = stOffNVURP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffNVURP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_mpbFdaState = stOffVURP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

		case stOffVURP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when entering this state
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_dbncTimerStrt = 0;
			_validPressPend = false;
			_dbncRlsTimerStrt = 0;	//At this point is NOT the expected 0!!
			_validReleasePend = false;	//At this point is NOT the expected FALSE!!
			_isLatched = false;			//At this point is NOT the expected FALSE!!
			_validUnlatchPend = false;//At this point is NOT the expected FALSE!!
			_srvcTimerStrt = 0;		//At this point is NOT the expected 0!!
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Execute this code only ONCE, when exiting this state
			}
			break;

	default:
		break;
	}

	return;
}

bool TmLtchMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(!_validPressPend){
			if(_dbncRlsTimerStrt != 0)
				_dbncRlsTimerStrt = 0;
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
		if(!_validReleasePend && _prssRlsCcl){
			if(_dbncTimerStrt != 0)
				_dbncTimerStrt = 0;
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

void TmLtchMPBttn::updValidUnlatchStatus(){
	if(_isLatched){
		if(_validPressPend){
			if(_tmRstbl)
				_srvcTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;
			_validPressPend = false;
		}
		if (((xTaskGetTickCount() / portTICK_RATE_MS) - _srvcTimerStrt) >= _srvcTime){
			_validUnlatchPend = true;
		}
	}

	return;
}

void TmLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
    TmLtchMPBttn* mpbObj = (TmLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	// Input/Output signals update
	mpbObj->updIsPressed();
	// Flags/Triggers calculation & update
 	mpbObj->updValidPressesStatus();
 	mpbObj->updValidUnlatchStatus();
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

HntdTmLtchMPBttn::HntdTmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const unsigned int &wrnngPrctg, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:TmLtchMPBttn(mpbttnPort, mpbttnPin, srvcTime, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _wrnngPrctg{wrnngPrctg <= 100?wrnngPrctg:100}
{
	_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;
}

bool HntdTmLtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
		if (!_mpbPollTmrHndl){
			_mpbPollTmrHndl = xTimerCreate(
				_mpbPollTmrName,  //Timer name
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

const bool HntdTmLtchMPBttn::getPilotOn() const{

    return _pilotOn;
}

const bool HntdTmLtchMPBttn::getWrnngOn() const{

    return _wrnngOn;
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

bool HntdTmLtchMPBttn::setKeepPilot(const bool &newKeepPilot){
	if(_keepPilot != newKeepPilot)
		_keepPilot = newKeepPilot;

	return _keepPilot;
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
			_pilotOn = true;
			_outputsChange = true;
		}
		else if(_isOn && _pilotOn){
			_pilotOn = false;
			_outputsChange = true;
		}
	}
	else{
		if(_pilotOn){
			_pilotOn = false;
			_outputsChange = true;
		}
	}

	return _pilotOn;
}

bool HntdTmLtchMPBttn::updWrnngOn(){
	if(_wrnngPrctg > 0){
		if (_isOn){
			if (((xTaskGetTickCount() / portTICK_RATE_MS) - _srvcTimerStrt) >= (_srvcTime - _wrnngMs)){
				if(_wrnngOn == false){
					_wrnngOn = true;
					_outputsChange = true;
				}
			}
			else if(_wrnngOn == true){
				_wrnngOn = false;
				_outputsChange = true;
			}
		}
		else if(_wrnngOn == true){
			_wrnngOn = false;
			_outputsChange = true;
		}
	}

	return _wrnngOn;
}

void HntdTmLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	HntdTmLtchMPBttn* mpbObj = (HntdTmLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	// Input/Output signals update
	mpbObj->updIsPressed();

	// Flags/Triggers calculation & update
 	mpbObj->updValidPressesStatus();
 	mpbObj->updValidUnlatchStatus();
 	// State machine state update
 	mpbObj->updFdaState();

 	mpbObj->updWrnngOn();
	mpbObj->updPilotOn();

	if (mpbObj->getOutputsChange()){
		if(mpbObj->getTaskToNotify() != NULL)
			xTaskNotifyGive(mpbObj->getTaskToNotify());
		mpbObj->setOutputsChange(false);
	}

	return;
}

//=========================================================================> Class methods delimiter

XtrnUnltchMPBttn::XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _unLtchBttn{unLtchBttn}
{
}

XtrnUnltchMPBttn::XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool XtrnUnltchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (pollDelayMs > 0){
		if (!_mpbPollTmrHndl){
			_mpbPollTmrHndl = xTimerCreate(
				_mpbPollTmrName,  //Timer name
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

void XtrnUnltchMPBttn::updValidUnlatchStatus(){

	if(_unLtchBttn != nullptr){
		if(_isLatched){
			if (_unLtchBttn->getIsOn()){
				_validUnlatchPend = true;
			}
		}
	}
	return;
}

void XtrnUnltchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	XtrnUnltchMPBttn* mpbObj = (XtrnUnltchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

	// Input/Output signals update
	mpbObj->updIsPressed();

	// Flags/Triggers calculation & update
 	mpbObj->updValidPressesStatus();
 	mpbObj->updValidUnlatchStatus();
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

DblActnLtchMPBttn::DblActnLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
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
                _mpbPollTmrName,  //Timer name
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

unsigned long DblActnLtchMPBttn::getScndModActvDly(){

	return _scndModActvDly;
}

void DblActnLtchMPBttn::scndModActn(){

	return;
}

void DblActnLtchMPBttn::scndModEndSttng(){

	return;
}

void DblActnLtchMPBttn::scndModStrtSttng(){

	return;
}

bool DblActnLtchMPBttn::setScndModActvDly(const unsigned long &newVal){
	bool result{false};

	if(newVal != _scndModActvDly){
		_scndModActvDly = newVal;
		result = true;
	}

	return result;
}

void DblActnLtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(_validPressPend || _validScndModPend){
				_mpbFdaState = stOffVPP;	//Start pressing timer
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
			}
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(!_isOn){
				_isOn = true;
				_outputsChange = true;
			}
			if(_validScndModPend){
//				_sldrTmrStrt = (xTaskGetTickCount() / portTICK_RATE_MS);
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
			if(_sttChng){
				// Placeholder
			}
			break;

		case stOnStrtScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){
				scndModStrtSttng();
//				if(_autoSwpDirOnPrss)
//					swapSldrDir();
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_mpbFdaState = stOnScndMod;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
			}
			break;

		case stOnScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			if(!_validReleasePend){
				//Operating in Second Mode
				//In this case Slider Mode
				scndModActn();
			}
			else{
				// MPB released, close Slider mode, move on to next state
				_mpbFdaState = stOnEndScndMod;
				setSttChng();
			}

			//Out: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
			}
			break;

		case stOnEndScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_scndModTmrStrt = 0;
			_validScndModPend = false;
			_mpbFdaState = stOnMPBRlsd;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				scndModEndSttng();
			}

			break;
		case stOnMPBRlsd:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				clrSttChng();
			}
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
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
			}
			break;

		case stOnTurnOff:
			//In: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
				clrSttChng();
			}
			//Do: >>---------------------------------->>
			_isOn = false;
			_outputsChange = true;
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				// Placeholder
			}
			break;

	default:
		break;
	}

	return;
}

bool DblActnLtchMPBttn::updValidPressPend(){
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
				_validReleasePend = false;	//Needed for the singular case of _sldrActvDly = 0
			}
			if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
				if(!_validScndModPend){
					_validPressPend = true;
					_validReleasePend = false;
					_prssRlsCcl = true;

				}
			}
		}
	}
	else{
		if(!_validReleasePend && _prssRlsCcl){
			if(_dbncTimerStrt != 0)
				_dbncTimerStrt = 0;
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

	return true;
}

void DblActnLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	DblActnLtchMPBttn* mpbObj = (DblActnLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

 	// Input/Output signals update
	mpbObj->updIsPressed();
	// Flags/Triggers calculation & update
 	mpbObj->updValidPressPend();
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

SldrLtchMPBttn::SldrLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long &dbncTimeOrigSett, const unsigned long int &strtDelay, const uint16_t initVal)
:DblActnLtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _otptCurVal{initVal}
{
	if(_otptCurVal < _otptValMin || _otptCurVal > _otptValMax)
		_otptCurVal = _otptValMin;	// Original development setup makes this outside limits situation impossible, as the limits are set to the full range of the data type used
}

SldrLtchMPBttn::~SldrLtchMPBttn()
{
}

bool SldrLtchMPBttn::begin(const unsigned long int &pollDelayMs) {
    bool result {false};
    BaseType_t tmrModResult {pdFAIL};

    if (pollDelayMs > 0){
        if (!_mpbPollTmrHndl){
            _mpbPollTmrHndl = xTimerCreate(
                _mpbPollTmrName,  //Timer name
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

uint16_t SldrLtchMPBttn::getOtptCurVal(){

	return _otptCurVal;
}

bool SldrLtchMPBttn::getOtptCurValIsMax(){

	return (_otptCurVal == _otptValMax);
}

bool SldrLtchMPBttn::getOtptCurValIsMin(){

	return (_otptCurVal == _otptValMin);
}

unsigned long SldrLtchMPBttn::getOtptSldrSpd(){

	return _otptSldrSpd;
}

uint16_t SldrLtchMPBttn::getOtptSldrStpSize(){

	return _otptSldrStpSize;
}

uint16_t SldrLtchMPBttn::getOtptValMax(){

	return _otptValMax;
}

uint16_t SldrLtchMPBttn::getOtptValMin(){

	return _otptValMin;
}

bool SldrLtchMPBttn::getSldrDirUp(){

	return _curSldrDirUp;
}

void SldrLtchMPBttn::scndModActn(){
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

void SldrLtchMPBttn::scndModEndSttng(){

	return;
}

void SldrLtchMPBttn::scndModStrtSttng(){
	if(_autoSwpDirOnPrss)
		swapSldrDir();

	return;
}

bool SldrLtchMPBttn::setOtptCurVal(const uint16_t &newVal){
	bool result{false};

	if(_otptCurVal != newVal){
		if(newVal >= _otptValMin && newVal <= _otptValMax){
			_otptCurVal = newVal;
			result = true;
		}
	}

	return result;
}

bool SldrLtchMPBttn::setOtptSldrSpd(const uint16_t &newVal){
	bool result{false};

	if(newVal != _otptSldrSpd){
		if(newVal > 0){	//The "Change Value Speed" must be a positive number representing the time in milliseconds between "Output Values" change the size of a "Value Step Quantity" while the MPB is being kept pressed
			_otptSldrSpd = newVal;
			result = true;
		}
	}

	return result;
}

bool SldrLtchMPBttn::setOtptSldrStpSize(const uint16_t &newVal){
	bool result{false};

	if(newVal != _otptSldrStpSize){
		if(newVal <= (_otptValMax - _otptValMin)){	//If newVal == (_otptValMax - _otptValMin) the slider will work as kind of an On/Off switch
			_otptSldrStpSize = newVal;
			result = true;
		}
	}

	return result;
}

bool SldrLtchMPBttn::setOtptValMax(const uint16_t &newVal){
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

bool SldrLtchMPBttn::setOtptValMin(const uint16_t &newVal){
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

bool SldrLtchMPBttn::_setSldrDir(const bool &newVal){
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

bool SldrLtchMPBttn::setSldrDirDn(){

	return _setSldrDir(false);
}

bool SldrLtchMPBttn::setSldrDirUp(){

	return _setSldrDir(true);
}

bool SldrLtchMPBttn::setSwpDirOnEnd(const bool &newVal){

	if(_autoSwpDirOnEnd != newVal)
		_autoSwpDirOnEnd = newVal;

	return _autoSwpDirOnEnd;
}

bool SldrLtchMPBttn::setSwpDirOnPrss(const bool &newVal){

	if(_autoSwpDirOnPrss != newVal)
		_autoSwpDirOnPrss = newVal;

	return _autoSwpDirOnEnd;
}

bool SldrLtchMPBttn::swapSldrDir(){

	return _setSldrDir(!_curSldrDirUp);
}

void SldrLtchMPBttn::updValidUnlatchStatus(){	//Placeholder for future development
	if(true){
		_validUnlatchPend = true;
	}

	return;
}

void SldrLtchMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	SldrLtchMPBttn* mpbObj = (SldrLtchMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);

 	// Input/Output signals update
	mpbObj->updIsPressed();
	// Flags/Triggers calculation & update
 	mpbObj->updValidPressPend();
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

DDlydLtchMPBttn::DDlydLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DblActnLtchMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DDlydLtchMPBttn::~DDlydLtchMPBttn()
{
}

bool DDlydLtchMPBttn::getIsOn2(){

	return _isOn2;
}

void DDlydLtchMPBttn::updFdaState(){

	return;
}

//=========================================================================> Class methods delimiter

VdblMPBttn::VdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:DbncdDlydMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _isOnDisabled{isOnDisabled}
{
}

VdblMPBttn::~VdblMPBttn()
{
}

void VdblMPBttn::clrStatus(){
    /*
    To Resume operation after a pause() without risking generating false "Valid presses" and "On" situations,
    several attributes must be reseted to "Start" values
    */
    DbncdMPBttn::clrStatus();	//This method might set the _outputsChange flag if the _isOn flag was set, as it will be changed to false.
    setIsVoided(false);

    return;
}

const bool VdblMPBttn::getIsEnabled() const{

    return _isEnabled;
}

const bool VdblMPBttn::getIsOnDisabled() const{

    return _isOnDisabled;
}

const bool VdblMPBttn::getIsVoided() const{

    return _isVoided;
}

bool VdblMPBttn::setIsEnabled(const bool &newEnabledValue){
	if(_isEnabled != newEnabledValue){
		if (!newEnabledValue){  //Changed to !Enabled (i.e. Disabled)
			pause();    //It's pausing the timer that keeps the inputs updated and calculates and updates the output flags... Flags must be updated for the disabled condition
			_isPressed = false;
			_validPressPend = false;
			_dbncTimerStrt = 0;
			if(_isOnDisabled){  //Set the _isOn flag to expected value
				if(_isOn == false){
					_isOn = true;
					_outputsChange = true;
				}
			}
			else{
				if (_isOn == true){
					_isOn = false;
					_outputsChange = true;
				}
			}
			if(_isVoided){
				_isVoided = false;
			   _outputsChange = true;
			}
		}
		else{
			clrStatus();
			resume();   //It's resuming the timer that keeps the inputs updated and calculates and updates the output flags... before this some conditions of timers and flags had to be insured
		}
		_isEnabled = newEnabledValue;
	}

	return _isEnabled;
}

bool VdblMPBttn::setIsOnDisabled(const bool &newIsOnDisabled){
	if(_isOnDisabled != newIsOnDisabled){
		_isOnDisabled = newIsOnDisabled;
	  if(!_isEnabled){
		  if(_isOn != _isOnDisabled){
			  _isOn = _isOnDisabled;
			  _outputsChange = true;
		  }
	  }
	}

	return _isOnDisabled;
}

bool VdblMPBttn::setIsVoided(const bool &newVoidValue){
    if(_isVoided != newVoidValue){
        _isVoided = newVoidValue;
        _outputsChange = true;
    }

    return _isVoided;
}

bool VdblMPBttn::enable(){

    return setIsEnabled(true);
}

bool VdblMPBttn::disable(){

    return setIsEnabled(false);
}

//=========================================================================> Class methods delimiter

TmVdblMPBttn::TmVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:VdblMPBttn(mpbttnPort, mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, isOnDisabled), _voidTime{voidTime}
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
			_mpbPollTmrName,  //Timer name
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

bool TmVdblMPBttn::setIsEnabled(const bool &newEnabledValue){
	if(_isEnabled != newEnabledValue){
		VdblMPBttn::setIsEnabled(newEnabledValue);
		if (newEnabledValue){  //Changed to Enabled
			clrStatus();
			setIsVoided(true);  	//For safety reasons if the mpb was disabled and re-enabled, it is set as voided, so if it was pressed when is was re-enabled there's no risk
									// of activating something unexpectedly. It'll have to be released and then pressed back to intentionally set it to ON.
			resume();   //It's resuming the timer that keeps the inputs updated and calculates and updates the output flags... before this some conditions of timers and flags had to be insured
		}
		_isEnabled = newEnabledValue;
		_outputsChange = true;
	}

	return _isEnabled;
}

bool TmVdblMPBttn::setIsVoided(const bool &newVoidValue){
    if(_isVoided != newVoidValue){
       if(newVoidValue){
           _voidTmrStrt = (xTaskGetTickCount() / portTICK_RATE_MS) - (_voidTime + 1);
       }
        _outputsChange = true;
        _isVoided = newVoidValue;
    }

    return _isVoided;
}

bool TmVdblMPBttn::setVoidTime(const unsigned long int &newVoidTime){
    bool result{false};

    if((newVoidTime != _voidTime) && (newVoidTime > _MinSrvcTime)){
        _voidTime = newVoidTime;
        result = true;
    }

    return result;
}

bool TmVdblMPBttn::updIsOn() {
	if (!_isVoided){
		if (_validPressPend){
			if(_isOn == false){
				_isOn = true;
				_outputsChange = true;
			}
		}
		else{
			if(_isOn == true){
				_isOn = false;
				_outputsChange = true;
			}
		}
	}
	else{
		if (_isOn == true){
			_isOn = false;
			_outputsChange = true;
		}
	}

	return _isOn;
}

bool TmVdblMPBttn::updIsPressed(){

    return DbncdDlydMPBttn::updIsPressed();
}

bool TmVdblMPBttn::updIsVoided(){
    //if it's pressed
        //if the pressing timer is running
            // if the pressing timer is greater than the debounceTime + strtDelay + voidTime
                //Set isVoided to true
                //Set isOn to false (the updateIsOn() will have to check _isVoided to prevent reverting back on)
    bool result {false};

    if(_validPressPend){
        if(_voidTmrStrt == 0){    //It was not previously pressed
            //Started to be pressed
            _voidTmrStrt = xTaskGetTickCount() / portTICK_RATE_MS;
        }
        else{
            if (((xTaskGetTickCount() / portTICK_RATE_MS) - _voidTmrStrt) >= (_voidTime)){ // + _dbncTimeTempSett + _strtDelay
                result = true;
            }
        }
    }
    else{
        _voidTmrStrt = 0;
    }
    if(_isVoided != result)
        _outputsChange = true;

    _isVoided = result;

    return _isVoided;
}

bool TmVdblMPBttn::updValidPressPend(){

//    return DbncdDlydMPBttn::updValidPressPend();
	return true;
}

void TmVdblMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCb){
    TmVdblMPBttn *mpbObj = (TmVdblMPBttn*)pvTimerGetTimerID(mpbTmrCb);
    mpbObj->updIsPressed();
    mpbObj->updValidPressPend();
    mpbObj->updIsVoided();
    mpbObj->updIsOn();
    if (mpbObj->getOutputsChange()){
        if(mpbObj->getTaskToNotify() != nullptr)
            xTaskNotifyGive(mpbObj->getTaskToNotify());
        mpbObj->setOutputsChange(false);
    }

    return;
}

//=========================================================================> Class methods delimiter
