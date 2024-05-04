/**
  ******************************************************************************
  * @file	: mpbAsSwitch_STM32.h
  * @brief	: Header file for the MpbAsSwitch_STM32 library classes
  *
  * The library builds several switch mechanisms replacements out of simple push buttons
  * or similar equivalent digital signal inputs.
  * By using just a push button (a.k.a. momentary switches or momentary buttons, _**MPB**_
  * for short from here on) the classes implemented in this library will manage,
  * calculate and update different parameters to **generate the behavior of standard
  * electromechanical switches**.
  *
  * @author	: Gabriel D. Goldman
  *
  * @date	: Created on: Nov 6, 2023
  * 			: Last modificationon:	29/04/2024
  *
  ******************************************************************************
  * @attention	This file is part of the Examples folder for the MPBttnAsSwitch_ESP32
  * library. All files needed are provided as part of the source code for the library.
  *
  ******************************************************************************
  */

#ifndef MPBASSWITCH_STM32_H_
#define MPBASSWITCH_STM32_H_

#include <stdint.h>
#include <string>
#include <stdio.h>

//===========================>> Next lines included for developing purposes, corresponding headers must be provided for the production platform/s
#define MCU_SPEC 1

#ifdef MCU_SPEC
	#ifndef __STM32F4xx_HAL_H
		#include "stm32f4xx_hal.h"
	#endif

	#ifndef __STM32F4xx_HAL_GPIO_H
		#include "stm32f4xx_hal_gpio.h"
	#endif
#endif
//===========================>> Previous lines included for developing purposes, corresponding headers must be provided for the production platform/s

//===========================>> BEGIN libraries used to avoid CMSIS wrappers
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
//===========================>> END libraries used to avoid CMSIS wrappers

#define _HwMinDbncTime 20  // Documented minimum wait time for a MPB signal to stabilize to consider it pressed or released (in milliseconds)
#define _StdPollDelay 10	// Reasonable time between polls for MPBs switches (in milliseconds)
#define _MinSrvcTime 100	// Minimum valid time value for service/active time for Time Latched MPBs to avoid stability issues relating to debouncing, releasing and other timed events
#define _InvalidPinNum 0xFFFF	// Value to give as "yet to be defined", the "Valid pin number" range and characteristics are development platform and environment dependable

/**
 * @brief Structure that holds all the required information to identify unequivocally a MCU's GPIO pin
 *
 * A GPIO pin identification is MCU and framework dependent, so the quantity and type of parameters
 * vary. This structure provides an interface to abstract them as a single identification parameter.
 *
 * @struct gpioPinId_t
 */
struct gpioPinId_t{	// Type used to keep GPIO pin identification as a single parameter, as platform independent as possible
	GPIO_TypeDef* portId;	/**< The port identification as a pointer to a GPIO_TypeDef information structure*/
	uint16_t pinNum;	/**< The number of pin represented as a one bit set binary with the set bit position indicating the pin number*/
};

typedef void (*fncPtrType)();
typedef  fncPtrType (*ptrToTrnFnc)();
//===========================>> BEGIN General use function prototypes

uint8_t singleBitPosNum(uint16_t mask);

//===========================>> END General use function prototypes

constexpr int genNxtEnumVal(const int &curVal, const int &increment){return (curVal + increment);}

//==========================================================>> BEGIN Classes declarations

/**
 * @brief	Base class, implements a Debounced Momentary Pushbutton.
 *
 * This class provides the resources needed to process a momentary digital input signal -as the one
 * provided by a MPB (momentary push button)- returning a clean signal to be used as a switch,
 * implementing the needed services to replace a wide range of physical related switch characteristics.
 *
 * @class	DbncdMPBttn
 */
class DbncdMPBttn {
protected:
	enum fdaDmpbStts {stOffNotVPP,
							stOffVPP,
							stOn,
							stOnVRP,
							stDisabled
	};
	const unsigned long int _stdMinDbncTime {_HwMinDbncTime};

	GPIO_TypeDef* _mpbttnPort{};
	uint16_t _mpbttnPin{};
	bool _pulledUp{};
	bool _typeNO{};
	unsigned long int _dbncTimeOrigSett{};

	unsigned long int _dbncRlsTimerStrt{0};
	unsigned long int _dbncRlsTimeTempSett{0};
	unsigned long int _dbncTimerStrt{0};
	unsigned long int _dbncTimeTempSett{0};
	void (*_fnWhnTrnOff)() {nullptr};
	void (*_fnWhnTrnOn)() {nullptr};
   bool _isEnabled{true};
	volatile bool _isOn{false};
   bool _isOnDisabled{false};
	volatile bool _isPressed{false};
	fdaDmpbStts _mpbFdaState {stOffNotVPP};
	TimerHandle_t _mpbPollTmrHndl {NULL};
	std::string _mpbPollTmrName {""};
	volatile bool _outputsChange {false};
	bool _prssRlsCcl{false};
	bool _sttChng {true};
	TaskHandle_t _taskToNotifyHndl {NULL};
	TaskHandle_t _taskWhileOnHndl{NULL};
	bool _validDisablePend{false};
	bool _validEnablePend{false};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};
	void clrSttChng();
	const bool getIsPressed() const;
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);
   bool setIsEnabled(const bool &newEnabledValue);
	void setSttChng();
	void _turnOff();
	void _turnOn();
	virtual void updFdaState();
	bool updIsPressed();
	virtual bool updValidPressesStatus();
public:
	/**
	 * @brief Default class constructor
	 *
	 */
	DbncdMPBttn();
	/**
	 * @brief Class constructor
	 *
	 * @param mpbttnPort GPIO port identification of the input signal pin
	 * @param mpbttnPin Pin id number of the input signal pin
	 * @param pulledUp (optional) boolean, indicates if the input pin must be configured as INPUT_PULLUP (true, default value), or INPUT_PULLDOWN (false), to calculate correctly the expected voltage level in the input pin. The pin is configured by the constructor so no previous programming is needed. The pin must be free to be used as a digital input, and must be a pin with an internal pull-up circuit, as not every GPIO pin has the option.
	 * @param typeNO optional boolean, indicates if the mpb is a Normally Open (NO) switch (true, default value), or Normally Closed (NC) (false), to calculate correctly the expected level of voltage indicating the mpb is pressed.
	 * @param dbncTimeOrigSett optional unsigned long integer (uLong), indicates the time (in milliseconds) to wait for a stable input signal before considering the mpb to be pressed (or not pressed). If no value is passed the constructor will assign the minimum value provided in the class, that is 20 milliseconds as it is an empirical value obtained in various published tests.
	 */
	DbncdMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Class constructor
	 *
	 * @param mpbttnPinStrct Structure defined to hold the GPIO port identification and the Pin id number as a single data unit
	 * @param pulledUp
	 * @param typeNO
	 * @param dbncTimeOrigSett
	 */
	DbncdMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Default destructor
	 *
	 */
	virtual ~DbncdMPBttn();
	/**
	 * @brief Attaches the instantiated object to a timer that monitors the input pin and updates the object status
	 *
	 * The frequency of that periodic monitoring is passed as a parameter in milliseconds, and is a value that must be small (frequent) enough to keep the object updated, but not so frequent that no other tasks can be executed. A default value is provided based on empirical values obtained in various published tests.
	 *
	 * @param pollDelayMs Unsigned long integer (ulong) optional, passes the time between polls in milliseconds.
	 * @return Boolean indicating if the object could be attached to a timer.
	 * @retval true: the object could be attached to a timer, or if it was already attached to a timer when the method was invoked.
	 * @retval false: the object could not create the needed timer, or the object could not be attached to it.
	 */
	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief Clears and resets flags and counters modified through the Turn-On/Turn-Off process.
	 * Resets object's attributes to initialization values to safely resume operations -by using the resume() method-
	 * after a pause(). This avoids risky behavior of the object due to dangling flags or partially ran time counters.
	 *
	 * @param clrIsOn Optional boolean value, indicates if the _isOn flag must be included to be cleared:
	 * - true (default value) includes the _isOn flag
	 * - false or not (false) excludes the _isOn flag
	 */
	void clrStatus(bool clrIsOn = true);
   /**
	 * @brief	Disables the input signal processing, ignoring the changes of its values.
    *
    * @retval	True: the object was enabled, the method invocation disabled it.
    * @retval	False: the object was disabled, the method made no changes in the status.
    */
	bool disable();
   /**
	 * @brief	Enables the input signal processing.
    *
    * @retval	True: the object was disabled, the method invocation enabled it.
    * @retval	False: the object was enabled, the method made no changes in the status.
    */
   bool enable();
	/**
	 * @brief Detaches the object from the timer that monitors the input pin/s and updates the object status. The timer daemon entry is deleted for the object.
	 *
	 * @return Boolean indicating the success of the operation
	 * @retval true: the object detachment procedure and timer entry removal was successful.
	 * @retval false: the object detachment and/or entry removal was rejected by the O.S..
	 *
	 */
	bool end();
	/**
	 * @brief 	Gets the current debounce time
	 *
	 * @return	The current debounce time in milliseconds
	 */
   const unsigned long int getCurDbncTime() const;
	/**
	 * @brief Returns a pointer to the function that was set to execute every time the _isOn flag is set (the object is signaling **On**)
	 *
	 * @return nullptr if there is no function set to execute
	 * @return a function pointer to the function set to execute through setFnWhnTrnOffPtr
	 */
   fncPtrType getFnWhnTrnOn();
	/**
	 * @brief Returns a pointer to the function that was set to execute every time the _isOn flag is reset (the object is signaling **Off**)
	 *
	 * @return nullptr if there is no function set to execute
	 * @return a function pointer to the function set to execute through setFnWhnTrnOnPtr
	 */
	fncPtrType  getFnWhnTrnOff();
   /**
	 * @brief Gets the value of the _isEnabled flag, indicating the **Enabled** status of the object.
	 *
    * @retval true: the object is in **Enabled state**.
    * @retval false: The object is in **Disabled state**.
    */
   const bool getIsEnabled() const;
   /**
	 * @brief Gets the value of the _isOn flag.
	 *
	 * The _isOn flag is one of the fundamental attributes of the object, it indicates if the object is in the **On** (true) or the **Off** (false) status.
	 * While other mechanism are provided to execute code when the status of the object changes, all but the _isOn flag value are optionally executed.
	 *
    * @retval true: the object is in **On state**.
    * @retval false: The object is in **Off state**.
    */
   const bool getIsOn () const;
   /**
	 * @brief Gets the value of the _isOnDisabled flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**, that might be changed when needed.
	 * In the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this flag value.
	 * The reasons to disable the ability to change the output, an keep it on either state until re-enabled are design and use dependent.
	 * The flag value might be changed by the use of the setIsOnDisabled
    *
    * @retval true: the object is configured to be set to the **On state** while it is in **Disabled state**.
    * @retval false: the object is configured to be set to the **Off state** while it is in **Disabled state**.
    */
   const bool getIsOnDisabled() const;
	/**
	 * @brief Gets the value of the _outputsChange flag.
	 *
	 * The instantiated objects include attributes linked to their computed state, which represent the behavior expected from their respective electromechanical simulated counterparts.
	 * When any of those attributes values change, the _outputsChange flag is set. The flag only signals changes have been done, not which flags, nor how many times changes have taken place.
	 * The _outputsChange flag must be set or reset through the setOutputsChange() method.
	 *
    * @retval true: any of the object's behavior flags have changed value since last **_outputsChange** flag was reseted.
    * @retval false: no object's behavior flags have changed value since last **_outputsChange** flag was reseted.
	 */
   const bool getOutputsChange() const;
   /**
	 * @brief Gets the task handle of the task  to be notified by the object when its output flags changes (indicating there have been changes in the outputs since last FreeRTOS notification). When the object is created, this value is set to **nullptr** and a valid TaskHandle_t value might be set by using the **setTaskToNotify()** method. The task notifying mechanism will not be used while the task handle keeps the **nullptr** value, in which case the solution implementation will have to use any of the other provided mechanisms to test the object status, and act accordingly.
	 *
    * @return The TaskHandle_t value of the task to be notified of the outputs change.
    * @retval NULL: there is no task configured to be notified of the output.
    */
	const TaskHandle_t getTaskToNotify() const;
	/**
	 * @brief Gets the task handle of the task  to be **resumed** when the object enters the **On state**, and will be **paused** when the  object enters the **Off state**.
	 *
	 * @return The TaskHandle_t value of the task to be resumed while the object is in **On state**.
    * @retval NULL: there is no task configured to be notified of the output.
	 */
	const TaskHandle_t getTaskWhileOn();
	bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Stops the software timer updating the calculation of the object internal flags.
	 *
	 * The timer will be kept for future use, but the flags will keep their last values and will not be updated until the timer is restarted with the `.resume()` method.
	 *
	 * @return true: the object's timer could be stopped by the O.S..
	 * @return false: the object's timer couldn't be stopped by the O.S..
	 *
	 */
	bool pause();
	/**
	 * @brief Resets the debounce time of the object to the value used at instantiation.
	 *
	 * In case the value was not specified at instantiation the default debounce time value will be used. As the time used at instantiation might be changed with the **setDbncTime()**, this method reverts the value.
	 *
	 * @retval true: the value could be reverted.
	 * @retval false: the value couldn't be reverted due to unexpected situations.
	 *
	 */
	bool resetDbncTime();
	bool resetFda();
	/**
	 * @brief Restarts the software timer updating the calculation of the object internal flags.
	 *
	 *  The timer will stop its function of computing the flags values after calling the `.pause()` method and will not be updated until the timer is restarted with this method.
	 *
	 * @retval true: the object's timer could be restarted by the O.S..
	 * @retval false: the object's timer couldn't be restarted by the O.S..
	 *
	 */
	bool resume();
	/**
	 * @brief Sets a new debounce time
	 *
	 *	Sets a new time for the debouncing period. The value must be equal or greater than the minimum empirical value set as a property for all the classes, 20 milliseconds. A long debounce time will produce a delay in the press event generation, making it less "responsive".
	 *
	 * @param newDbncTime	unsigned long integer, the new debounce value for the object.
	 * @return	A boolean indicating if the debounce time change was successful
	 * @retval true: the new value is in the accepted range and the change was made.
	 * @retval false: the value was already in use, or was out of the accepted range, no change was made.
	 */
	bool setDbncTime(const unsigned long int &newDbncTime);
	bool setFnWhnTrnOffPtr(void(*newFnWhnTrnOff)());
	bool setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)());
   bool setIsOnDisabled(const bool &newIsOnDisabled);
   /**
	 * @brief Sets the value of the flag indicating if a change took place in any of the output flags (IsOn included).
	 *
	 * The usual path for the _outputsChange flag is to be set to true by any method changing an output flag, the callback function signaled to take care of the hardware actions because of this changes clears back _outputsChange after taking care of them. In the unusual case the developer wants to "intercept" this sequence, this method is provided to set (true) or clear (false) _outputsChange value.
    *
    * @param newOutputChange the new value to set the _outputsChange flag to.
    *
    * @retval true: The value change was successful.
    * @retval false: The value held by _outputsChange and the newOutputChange parameter are the same, no change was then produced.
    */
   bool setOutputsChange(bool newOutputChange);
   /**
	 * @brief Sets the pointer to the task to be notified by the object when its output flags changes.
	 *
	 * When the object is created, this value is set to **nullptr** and a valid TaskHandle_t value might be set by using this method. The task notifying mechanism will not be used while the task handle keeps the **nullptr** value. After the TaskHandle value is set it might be changed to point to other task.
    *
    * @param newHandle A valid task handle of an actual existent task/thread running. There's no provided exception mechanism for dangling pointer errors caused by a pointed task being deleted and/or stopped.
    * @retval true: A TaskHandle_t type was passed to the object to be it's new pointer to the task to be messaged when a change in the output flags occur. There's no checking for the validity of the pointer, if it refers to an active task/thread whatsoever.
	 * @retval false: The value passed to the method was **nullptr**, and that's the value will be stored, so the whole RTOS messaging mechanism won't be used.
    *
    */
	bool setTaskToNotify(TaskHandle_t newHandle);
	bool setTaskWhileOn(const TaskHandle_t &newTaskHandle);
};

//==========================================================>>

/**
 * @brief Subclass, implements a Debounced Delayed Momentary Pushbutton.
 *
 * The **Debounced Delayed Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released. The reasons to add the delay are design related and are usually used to avoid unintentional presses, or to give some equipment (load) that needs time between repeated activations the benefit of the pause. If the push button is released before the delay configured, no press is registered at all. The delay time in this class as in the other that implement it, might be zero (0), defined by the developer and/or modified in runtime.
 *
 * @class
 */
class DbncdDlydMPBttn: public DbncdMPBttn{
protected:
    unsigned long int _strtDelay {0};

    virtual bool updValidPressesStatus();
public:
    DbncdDlydMPBttn();
    DbncdDlydMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    DbncdDlydMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Gets the current value of start delay parameter.
     *
     * Returns the current value of time used by the object to rise the isOn flag, after the debouncing process ends, in milliseconds. If the MPB is released before completing the debounce **and** the delay time, no press will be detected by the object, and the isOn flag will not be rised. The original value for the delay process used at instantiation time might be changed with the **setStrtDelay()** method, so this method is provided to get the current value in use.
     *
     * @return The current delay time, in milliseconds, being used before rising the isOn flag, after the debounce process of the current object.
     */
    unsigned long int getStrtDelay();
    bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    bool setStrtDelay(const unsigned long int &newStrtDelay);
};

//==========================================================>>

class LtchMPBttn: public DbncdDlydMPBttn{
protected:
	enum fdaLmpbStts {stOffNotVPP,
		stOffVPP,
		stOnNVRP,
		stOnVRP,
		stLtchNVUP,
		stLtchdVUP,
		stOffVUP,
		stOffNVURP,
		stOffVURP,
		stDisabled
	};
	bool _isLatched{false};
	fdaLmpbStts _mpbFdaState {stOffNotVPP};
	bool _trnOffASAP{true};
	bool _validUnlatchPend{false};
	bool _validUnlatchRlsPend{false};

	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
	virtual void updFdaState();
	virtual void updValidUnlatchStatus() = 0;

	virtual void stOffNotVPP_In(){};	//Setting the startup values for the FDA
	virtual void stOffNotVPP_Out(){};
	virtual void stOffNVURP_Do(){};
	virtual void stOffVPP_Out(){};
	virtual void stOffVURP_Out(){};
   virtual void stOnNVRP_Do(){};
	virtual void stLtchNVUP_Do(){};
	virtual void stDisabled_In(){};
   virtual void stDisabled_Out(){};
public:
	LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	LtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	void clrStatus(bool clrIsOn = true);
	const bool getIsLatched() const;
	bool getTrnOffASAP();
	const bool getUnlatchPend() const;
	bool resetFda();
	bool setTrnOffASAP(const bool &newVal);
	bool setUnlatchPend(const bool &newVal);
   bool unlatch();

	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class TgglLtchMPBttn: public LtchMPBttn{
protected:
	virtual void updValidUnlatchStatus();
	virtual void stOffNVURP_Do();
public:
	TgglLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	TgglLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
};

//==========================================================>>

class TmLtchMPBttn: public LtchMPBttn{
protected:
    bool _tmRstbl {true};
    unsigned long int _srvcTime {};
    unsigned long int _srvcTimerStrt{0};

    virtual void updValidUnlatchStatus();
    virtual void stOffNotVPP_Out();
    virtual void stOffVPP_Out();
    virtual void stOffVURP_Out();
public:
    TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    TmLtchMPBttn(gpioPinId_t mpbttnPinStrct, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    void clrStatus(bool clrIsOn = true);
    const unsigned long int getSrvcTime() const;
    bool setSrvcTime(const unsigned long int &newSrvcTime);
    bool setTmerRstbl(const bool &newIsRstbl);
};

//==========================================================>>

class HntdTmLtchMPBttn: public TmLtchMPBttn{

protected:
	bool _keepPilot{false};
	bool _pilotOn{false};
	unsigned long int _wrnngMs{0};
	bool _wrnngOn {false};
	unsigned int _wrnngPrctg {0};

	bool _validWrnngSetPend{false};
	bool _validWrnngResetPend{false};
	bool _validPilotSetPend{false};
	bool _validPilotResetPend{false};

	virtual void stOffNotVPP_In();
	virtual void stOffVPP_Out();
   virtual void stOnNVRP_Do();
	virtual void stLtchNVUP_Do();
	virtual void stDisabled_In();


   static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
	bool updPilotOn();
	bool updWrnngOn();
public:
	HntdTmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &actTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	HntdTmLtchMPBttn(gpioPinId_t mpbttnPinStrct, const unsigned long int &actTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	void clrStatus(bool clrIsOn = true);
	const bool getPilotOn() const;
	const bool getWrnngOn() const;
	bool setKeepPilot(const bool &newKeepPilot);
	bool setSrvcTime(const unsigned long int &newActTime);
	bool setWrnngPrctg (const unsigned int &newWrnngPrctg);

	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class XtrnUnltchMPBttn: public LtchMPBttn{

protected:
    DbncdDlydMPBttn* _unLtchBttn {nullptr};
    bool _xtrnUnltchPRlsCcl {false};

// 	virtual void stOffNotVPP_In();
 	virtual void stOffNVURP_Do();
 	virtual void updValidUnlatchStatus();
public:
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    XtrnUnltchMPBttn(gpioPinId_t mpbttnPinStrct,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    XtrnUnltchMPBttn(gpioPinId_t mpbttnPinStrct,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);	// Duplicate code? Refers to the LtchMPBttn::mpbPollCallback?? Check Gaby
    void clrStatus(bool clrIsOn = true);
};

//==========================================================>>

class DblActnLtchMPBttn: public LtchMPBttn{
	friend constexpr int genNxtEnumVal(const int &curVal, const int &increment);

protected:
	enum fdaDALmpbStts{
		stOffNotVPP = 0,
		stOffVPP = genNxtEnumVal(stOffNotVPP, 100),
		stOnMPBRlsd = genNxtEnumVal(stOffVPP,100),
		//--------
		stOnStrtScndMod = genNxtEnumVal(stOnMPBRlsd,100),
		stOnScndMod = genNxtEnumVal(stOnStrtScndMod,100),
		stOnEndScndMod = genNxtEnumVal(stOnScndMod,100),
		//--------
		stOnTurnOff = genNxtEnumVal(stOnEndScndMod,100),
		//--------
		stDisabled = genNxtEnumVal(stOnTurnOff,100)

	};
	fdaDALmpbStts _mpbFdaState {stOffNotVPP};
	unsigned long _scndModActvDly {2000};
	unsigned long _scndModTmrStrt {0};
	bool _validScndModPend{false};

	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
   virtual void stDisabled_In(){};
	virtual void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do() = 0;
   virtual void stOnEndScndMod_Out();
	virtual void updFdaState();
	virtual bool updValidPressesStatus();
   virtual void updValidUnlatchStatus();

public:
	DblActnLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	DblActnLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	~DblActnLtchMPBttn();
   void clrStatus(bool clrIsOn = true);
	unsigned long getScndModActvDly();
	bool setScndModActvDly(const unsigned long &newVal);

   bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class DDlydDALtchMPBttn: public DblActnLtchMPBttn{
protected:
   bool _isOn2{false};

   virtual void stDisabled_In();
   virtual void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do();
   virtual void stOnEndScndMod_Out();
public:
   DDlydDALtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   DDlydDALtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   ~DDlydDALtchMPBttn();
   void clrStatus(bool clrIsOn = true);
   bool getIsOn2();
};

//==========================================================>>

class SldrDALtchMPBttn: public DblActnLtchMPBttn{

protected:
	bool _autoSwpDirOnEnd{true};	// Changes slider direction when reaches _otptValMax or _otptValMin
	bool _autoSwpDirOnPrss{false};// Changes slider direction each time it enters slider mode
	bool _curSldrDirUp{true};
	uint16_t _otptCurVal{};
	unsigned long _otptSldrSpd{1};
	uint16_t _otptSldrStpSize{0x01};
	uint16_t _otptValMax{0xFFFF};
	uint16_t _otptValMin{0x00};
//	unsigned long _sldrTmrNxtStrt{0};
//	unsigned long _sldrTmrRemains{0};

   void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do();
	bool _setSldrDir(const bool &newVal);
public:
   SldrDALtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
   SldrDALtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
   ~SldrDALtchMPBttn();
   void clrStatus(bool clrIsOn = true);
	uint16_t getOtptCurVal();
   bool getOtptCurValIsMax();
   bool getOtptCurValIsMin();
	uint16_t getOtptValMax();
	uint16_t getOtptValMin();
	unsigned long getOtptSldrSpd();
	uint16_t getOtptSldrStpSize();
	bool getSldrDirUp();
	bool setOtptCurVal(const uint16_t &newVal);
	bool setOtptValMax(const uint16_t &newVal);
	bool setOtptValMin(const uint16_t &newVal);
	bool setOtptSldrStpSize(const uint16_t &newVal);
	bool setOtptSldrSpd(const uint16_t &newVal);
	bool setSldrDirDn();
	bool setSldrDirUp();
	bool setSwpDirOnEnd(const bool &newVal);
	bool setSwpDirOnPrss(const bool &newVal);
	bool swapSldrDir();
};

//==========================================================>>

class VdblMPBttn: public DbncdDlydMPBttn{
protected:
	enum fdaVmpbStts{
 		stOffNotVPP,
 		stOffVPP,
 		stOnNVRP,
		//--------
		stOnVVP,
		stOnVddNVUP,
		stOffVddNVUP,
		stOffVddVUP,
		stOffUnVdd,
		//--------
 		stOnVRP,
		stOnTurnOff,
		stOff,
		//--------
		stDisabled
 	};
 	fdaVmpbStts _mpbFdaState {stOffNotVPP};

    bool _isVoided{false};
    bool _validVoidPend{false};
    bool _validUnvoidPend{false};

    static void mpbPollCallback(TimerHandle_t mpbTmrCb);
    bool setVoided(const bool &newVoidValue);
    virtual void stDisabled_In();
    virtual void stDisabled_Out();
    virtual void stOffNotVPP_In(){};
    virtual void stOffVPP_Do(){};	// This provides a setting point for the voiding mechanism to be started
    virtual void stOffVddNVUP_Do(){};	//This provides a setting point for calculating the _validUnvoidPend
    virtual void updFdaState();
    virtual bool updVoidStatus() = 0;
public:
    VdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    VdblMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    virtual ~VdblMPBttn();
    void clrStatus(bool clrIsOn = true);
    const bool getIsVoided() const;
    bool setIsNotVoided();
    bool setIsVoided();
};

//==========================================================>>

class TmVdblMPBttn: public VdblMPBttn{
protected:
    unsigned long int _voidTime;
    unsigned long int _voidTmrStrt{0};

    virtual void stOffNotVPP_In();
    virtual void stOffVddNVUP_Do();	//This provides a setting point for calculating the _validUnvoidPend
    virtual void stOffVPP_Do();	// This provides a setting point for the voiding mechanism to be started
    bool updIsPressed();
    virtual bool updVoidStatus();
public:
    TmVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    TmVdblMPBttn(gpioPinId_t mpbttnPinStrct, unsigned long int voidTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    virtual ~TmVdblMPBttn();
    void clrStatus();
    const unsigned long int getVoidTime() const;
    bool setVoidTime(const unsigned long int &newVoidTime);

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

#endif /* MPBASSWITCH_STM32_H_ */
