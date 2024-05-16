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
  * @date	: Created on: 06/11/2023
  * 			: Last modification:	05/05/2024
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
//#include "queue.h"
//#include "semphr.h"
//#include "event_groups.h"
//===========================>> END libraries used to avoid CMSIS wrappers

#define _HwMinDbncTime 20  // Documented minimum wait time for a MPB signal to stabilize to consider it pressed or released (in milliseconds)
#define _StdPollDelay 10	// Reasonable time between polls for MPBs switches (in milliseconds)
#define _MinSrvcTime 100	// Minimum valid time value for service/active time for Time Latched MPBs to avoid stability issues relating to debouncing, releasing and other timed events
#define _InvalidPinNum 0xFFFF	// Value to give as "yet to be defined", the "Valid pin number" range and characteristics are development platform and environment dependable

#ifndef GPIOPINID_T
#define GPIOPINID_T
/**
 * @brief Type used to keep GPIO pin identification as a single parameter, independently of the platform requirements.
 *
 * GPIO pin identification is hardware and development environment framework dependents, for some platforms it needs one, some two, some more parameters, and each one of these parameters type depend once again on the platform. This type is provided to define each pin referenced as a single parameter for class methods and attributes declarations, as platform independent as possible.
 *
 * struct
 */
struct gpioPinId_t{	//
	GPIO_TypeDef* portId;	/**< The port identification as a pointer to a GPIO_TypeDef information structure*/
	uint16_t pinNum;	/**< The number of the port pin represented as a one bit set binary with the set bit position indicating the pin number*/
};
#endif	//GPIOPINID_T

// Definition workaround to let a function/method return value to be a function pointer
typedef void (*fncPtrType)();
typedef  fncPtrType (*ptrToTrnFnc)();

//===========================>> BEGIN General use function prototypes
uint8_t singleBitPosNum(uint16_t mask);
//===========================>> END General use function prototypes

constexpr int genNxtEnumVal(const int &curVal, const int &increment){return (curVal + increment);}

//==========================================================>> BEGIN Classes declarations

/**
 * @brief Base class, implements a Debounced Momentary Push Button (**D-MPB**).
 *
 * This class provides the resources needed to process a momentary digital input signal -as the one provided by a MPB (momentary push button)- returning a clean signal to be used as a switch, implementing the needed services to replace a wide range of physical related switch characteristics: Debouncing, deglitching, disabling.
 *
 * @class DbncdMPBttn
 *
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
	 * @param pulledUp (Optional) boolean, indicates if the input pin must be configured as INPUT_PULLUP (true, default value), or INPUT_PULLDOWN (false), to calculate correctly the expected voltage level in the input pin. The pin is configured by the constructor so no previous programming is needed. The pin must be free to be used as a digital input, and must be a pin with an internal pull-up circuit, as not every GPIO pin has the option.
	 * @param typeNO (Optional) boolean, indicates if the MPB is a Normally Open (NO) switch (true, default value), or Normally Closed (NC) (false), to calculate correctly the expected level of voltage indicating the MPB is pressed.
	 * @param dbncTimeOrigSett (Optional) unsigned long integer (uLong), indicates the time (in milliseconds) to wait for a stable input signal before considering the MPB to be pressed (or not pressed). If no value is passed the constructor will assign the minimum value provided in the class, that is 20 milliseconds as it is an empirical value obtained in various published tests.
	 *
	 */
	DbncdMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Class constructor
	 *
	 * @param mpbttnPinStrct GPIO port identification and the Pin id number defined as a single gpioPinId_t parameter.
	 *
	 * For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
	 *
	 */
	DbncdMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Default virtual destructor
	 *
	 */
	virtual ~DbncdMPBttn();
	/**
	 * @brief Attaches the instantiated object to a timer that monitors the input pins and updates the object status.
	 *
	 * The frequency of the periodic monitoring is passed as a parameter in milliseconds, and is a value that must be small (frequent) enough to keep the object updated, but not so frequent that waste unneeded resources from other tasks. A default value is provided based on empirical values obtained in various published tests.
	 *
	 * @param pollDelayMs (Optional) unsigned long integer (ulong), the time between polls in milliseconds.
	 *
	 * @return Boolean indicating if the object could be attached to a timer.
	 * @retval true: the object could be attached to a timer -or if it was already attached to a timer when the method was invoked-.
	 * @retval false: the object could not create the needed timer, or the object could not be attached to it.
	 *
	 */
	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief Clears and resets flags, timers and counters modified through the object's signals processing.
	 *
	 * Resets object's attributes to initialization values to safely resume operations, either after pausing the timer, enabling the object after disabling it or any disruptive activity that might generate unexpected distorsions. This avoids risky behavior of the object due to dangling flags or partially ran time counters.
	 *
	 * @param clrIsOn Optional boolean value, indicates if the _isOn flag must be included to be cleared:
	 * - true (default value) includes the isOn flag.
	 * - false excludes the isOn flag.
	 *
	 */
	void clrStatus(bool clrIsOn = true);
   /**
	 * @brief Disables the input signal processing, ignoring the changes of its values.
	 *
	 * Invoking the disable() method sends a message to the object requesting it to enter the **Disabled state**. Due to security and stability of the object's behavior the message will not be processed at every stage of the input signals detection and output computations, but in every safe and stable stage. When the message is processed the object will:
	 * - Stop all input signal reading.
	 * - Stop any new output flag computation.
	 * - Reset all output flag values.
	 * - Force the isOn flag according to the **isOnDisabled** flag setting.
	 * - Keep this **Disabled state** behavior until an enabling message is received through an **enable()** method.
    *
    * @retval true: the object was enabled, the method invocation disabled it.
    * @retval false: the object was disabled, the method made no changes in the status.
    *
    */
	bool disable();
   /**
	 * @brief Enables the input signal processing.
	 *
	 * Invoking the enable() method on a object in **Disabled state** sends it a message requesting to resume it's normal operation. The execution of the re-enabling of the object implies:
	 * - Resuming all input signals reading.
	 * - Resuming all output flag computation from the "fresh startup" state, including clearing the **isOn state**
	 * - Due to strict security enforcement the object will not be allowed to enter the **Enabled state** if the MPB was pressed when the enable message was received and until a MPB release is efectively detected.
    *
    * @retval true: the object's input signal processing was disabled, the method invocation enabled it.
    * @retval false: the object's input signal processing was enabled, the method made no changes in the status.
    */
   bool enable();
	/**
	 * @brief Detaches the object from the timer that monitors the input pins, compute and updates the object's status. The timer daemon entry is deleted for the object.
	 *
	 * The immediate detachment of the object from the timer that keeps it's state updated implies that the object's state will be kept, whatever that state is it. If a certain status is preferred some of the provided methods should be used for that including clrStatus(), resetFda(), disable(), setIsOnDisabled(), etc. Also consider that if a task is set to be executed while the object is in **On state**, the **end()** invocation wouldn't affect that task execution state.
	 *
	 * @return Boolean indicating the success of the operation
	 * @retval true: the object detachment procedure and timer entry removal was successful.
	 * @retval false: the object detachment and/or entry removal was rejected by the O.S..
	 *
	 */
	bool end();
	/**
	 * @brief Gets the current debounce time set for the object.
	 *
	 * The original value for the debouncing process used at instantiation time might be changed with the **setDbncTime(const unsigned long int)** or with the **resetDbncTime()** methods. This method gets the current value of the attribute in use.
	 *
	 * @return The current debounce time in milliseconds
	 */
   const unsigned long int getCurDbncTime() const;
	/**
	 * @brief Gets the function that is set to execute every time the object enters the **Off State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOffPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **Off State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **Off State**.
	 *
	 */
	fncPtrType  getFnWhnTrnOff();
	/**
	 * @brief Gets the function that is set to execute every time the object enters the **On State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOnPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **On State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **On State**.
	 *
	 */
   fncPtrType getFnWhnTrnOn();
   /**
	 * @brief Gets the value of the isEnabled flag, indicating the **Enabled** or **Disabled** status of the object.
	 *
	 * The isEnabled flag might be modified by the enable() and the disable() methods.
	 *
    * @retval true: the object is in **Enabled state**.
    * @retval false: The object is in **Disabled state**.
    */
   const bool getIsEnabled() const;
   /**
	 * @brief Gets the value of the **isOn** flag.
	 *
	 * The **isOn** flag is the fundamental attribute of the object, it might be considered the "Raison d'etre" of all this classes design: the isOn signal is not just the detection of an expected voltage value at a mcu pin, but the combination of that voltage level, filtered and verified, for a determined period of time and until a new event modifies that situation.  While other mechanism are provided to execute code when the status of the object changes, all but the **isOn** flag value update are optionally executed.
	 *
    * @retval true: The object is in **On state**.
    * @retval false: The object is in **Off state**.
    *
    */
   const bool getIsOn () const;
   /**
	 * @brief Gets the value of the **isOnDisabled** flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**. That might be changed when needed.
	 * In the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this flag's value.
	 * The reasons to disable the ability to change the output, and keep it on either state until re-enabled are design and use dependent.
	 * The flag value might be changed by the use of the setIsOnDisabled() method.
    *
    * @retval true: the object is configured to be set to the **On state** while it is in **Disabled state**.
    * @retval false: the object is configured to be set to the **Off state** while it is in **Disabled state**.
    *
    */
   const bool getIsOnDisabled() const;
	/**
	 * @brief Gets the value of the **outputsChange** flag.
	 *
	 * The instantiated objects include attributes linked to their computed state, which represent the behavior expected from their respective electromechanical simulated counterparts.
	 * When any of those attributes values change, the **outputsChange** flag is set. The flag only signals changes have been done, not which flags, nor how many times changes have taken place, since the last **outputsChange** flag reset.
	 * The **outputsChange** flag must be reset (or set if desired) through the setOutputsChange() method.
	 *
    * @retval true: any of the object's behavior flags have changed value since last time **outputsChange** flag was reseted.
    * @retval false: no object's behavior flags have changed value since last time **outputsChange** flag was reseted.
	 */
   const bool getOutputsChange() const;
   /**
	 * @brief Gets the task to be notified by the object when its output flags changes.
	 *
	 * The task handle of the task to be notified by the object when its **outputsChange** flag is set (see getOutputsChange()) holds a **NULL** when the object is created. A valid task's TaskHandle_t value might be set by using the setTaskToNotify() method, as many times as needed, and even set back to **NULL** to disable the task notification mechanism.
	 *
    * @return TaskHandle_t value of the task to be notified of the outputs change.
    * @retval NULL: there is no task configured to be notified of the outputs change.
    *
    */
	const TaskHandle_t getTaskToNotify() const;
	/**
	 * @brief Gets the task to be run while the object is in the **On state**.
	 *
	 * Gets the task handle of the task to be **resumed** every time the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechamism dependant of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * @return The TaskHandle_t value of the task to be resumed while the object is in **On state**.
    * @retval NULL  if there is no task configured to be resumed while the object is in **On state**.
    *
    * @warning Free-RTOS has no mechanism implemented to notify a task that it is about to be set in **paused** state, so there is no way to that task to ensure it will be set to pause in an orderly fashion. So the task to be designated to be used by this mechanism has to be task that can withold being interrupted at any point of it's execution, an be restarted from that same point next time the **isOn** flag is set. For tasks that might need attaching resources or other issues every time it is resumed and releasing resources of any kind before being **paused**, using the function attached by using **setFnWhnTrnOnPtr()** to gain control of the resources before resuming a task, and the function attached by using **setFnWhnTrnOffPtr()** to release the resources and pause the task in an orderly fashion.
	 */
	const TaskHandle_t getTaskWhileOn();
	/**
	 * @brief Initializes an object instantiated by the default constructor
	 *
	 * All the parameters correspond to the non-default constructor of the class, DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
	 *
	 */
	bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Initializes an object instantiated by the default constructor
	 *
	 * All the parameters correspond to the non-default constructor of the class, DbncdMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int)
	 *
	 */
	bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Pauses the software timer updating the computation of the object's internal flags value.
	 *
	 * The immediate stop of the timer that keeps the object's state updated implies that the object's state will be kept, whatever that state is it. The same consideration as the end() method applies referring to options to modify the state in which the object will be while in the **Pause state**.
	 *
	 * @retval true: the object's timer could be stopped by the O.S..
	 * @retval false: the object's timer couldn't be stopped by the O.S..
	 *
	 */
	bool pause();
	/**
	 * @brief Resets the debounce process time of the object to the value used at instantiation.
	 *
	 *  The debounce process time used at instantiation might be changed with the setDbncTime() as needed, as many times as needed. This method reverts the value to the instantiation time value. In case the value was not specified at instantiation time the default debounce time value will be used.
	 *
	 * @retval true: the value could be reverted.
	 * @retval false: the value couldn't be reverted due to unexpected situations.
	 *
	 */
	bool resetDbncTime();
	/**
	 * @brief Resets the MPB behavior automaton to it's **Initial or Start State**
	 *
	 * This method is provided for security and for error handling purposes, so that in case of unexpected situations detected, the driving **Deterministic Finite Automaton** used to compute the MPB objects states might be reset to it's initial state to safely restart it, usually as part of an **Error Handling** rutine.
	 *
	 * @retval true always it is invoked.
	 */
	bool resetFda();
	/**
	 * @brief Restarts the software timer updating the calculation of the object internal flags.
	 *
	 *  The timer will stop calling the functions for computing the flags values after calling the pause() method and will not be updated until the timer is restarted with this method.
	 *
	 * @retval true: the object's timer could be restarted by the O.S..
	 * @retval false: the object's timer couldn't be restarted by the O.S..
	 *
	 */
	bool resume();
	/**
	 * @brief Sets a new debounce time.
	 *
	 *	Sets a new time for the debouncing period. The value must be equal or greater than the minimum empirical value set as a property for all the classes, 20 milliseconds. A long debounce time will produce a delay in the press event generation, making it less "responsive".
	 *
	 * @param newDbncTime unsigned long integer, the new debounce value for the object.
	 *
	 * @return	A boolean indicating if the debounce time change was successful.
	 * @retval true: the new value is in the accepted range and the change was made.
	 * @retval false: the value was the already in use, or was out of the accepted range, no change was made.
	 */
	bool setDbncTime(const unsigned long int &newDbncTime);
	/**
	 * @brief Sets the function that will be called to execute every time the object enters the **Off State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOff Function pointer to the function intended to be called when the object enters the **Off State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 *
	 * @retval true: the passed parameter was different to the previous attribute setting, the new value was set.
	 * @retval false: the passed parameter was equal to the previous attribute setting, no new value was set.
	 */
	bool setFnWhnTrnOffPtr(void(*newFnWhnTrnOff)());
	/**
	 * @brief Sets the function that will be called to execute every time the object enters the **On State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOn: function pointer to the function intended to be called when the object enters the **On State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 *
	 * @retval true: the passed parameter was different to the previous attribute setting, the new value was set.
	 * @retval false: the passed parameter was equal to the previous attribute setting, no new value was set.
	 */
	bool setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)());
   /**
	 * @brief Sets the value of the **isOnDisabled** flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**. That might be changed as needed.
	 * While in the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this flag value.
	 *
	 * @note The reasons to disable the ability to change the output, and keep it on either state until re-enabled are design and use dependent, being an obvious one security reasons, disabling the ability of the users to manipulate the switch while keeping the desired **On/Off state**. A simple example would be to keep a light on in a place where a meeting is taking place, disabling the lights switches and keeping the **On State**. Another obvious one would be to keep a machine off while servicing it's internal mechanisms, disabling the possibility of turning it on.
    *
    * @retval true: the object is configured to be set to the **On state** while it is in **Disabled state**.
    * @retval false: the object is configured to be set to the **Off state** while it is in **Disabled state**.
    *
    */
   bool setIsOnDisabled(const bool &newIsOnDisabled);
   /**
	 * @brief Sets the value of the flag indicating if a change took place in any of the output flags (IsOn included).
	 *
	 * The usual path for the **outputsChange** flag is to be set to true by any method changing an output flag, the callback function signaled to take care of the hardware actions because of this changes clears back **outputsChange** after taking care of them. In the unusual case the developer wants to "intercept" this sequence, this method is provided to set (true) or clear (false) outputsChange value.
    *
    * @param newOutputChange The new value to set the **outputsChange** flag to.
    *
    * @retval true: The value change was successful.
    * @retval false: The value held by **outputsChange** and the newOutputChange parameter are the same, no change was then produced.
    */
   bool setOutputsChange(bool newOutputChange);
   /**
	 * @brief Sets the pointer to the task to be notified by the object when its output flags changes.
	 *
	 * When the object is created, this value is set to **NULL**, and a valid TaskHandle_t value might be set by using this method. The task notifying mechanism will not be used while the task handle keeps the **NULL** value, in which case the solution implementation will have to use any of the other provided mechanisms to test the object status, and act accordingly. After the TaskHandle value is set it might be changed to point to other task.
	 *
    * @param newHandle A valid task handle of an actual existent task/thread running. There's no provided exception mechanism for dangling pointer errors caused by a pointed task being deleted and/or stopped.
    *
    * @retval true: A TaskHandle_t type was passed to the object to be it's new pointer to the task to be messaged when a change in the output flags occur. There's no checking for the validity of the pointer, if it refers to an active task/thread whatsoever.
	 * @retval false: The value passed to the method was **NULL**, and that's the value will be stored, so the whole RTOS messaging mechanism won't be used.
    *
    */
	bool setTaskToNotify(TaskHandle_t newHandle);
	/**
	 * @brief Sets the task to be run while the object is in the **On state**.
	 *
	 * Sets the task handle of the task to be **resumed** when the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 * Setting the value to NULL will disable the task execution mechanism off.
	 *
	 * @retval true: the newTaskHandle was different to the previous value and the change was made
    * @retval false: both newTaskHandle and the previous set value where identical, no change was made.
    *
    * @note Consider the implications of the task that's going to get suspended every time the MPB goes to the **Off state**, so that the the task to be run might be interrupted at any point of its execution. This implies that the task must be designed with that consideration in mind to avoid dangerous situations generated by a task not completely done when suspended.
    * @warning Take special consideration about the implications of the execution **priority** of the task to be executed while the MPB is in **On state** and its relation to the priority of the calling task, as it might affect the normal execution of the application.
	 */
	bool setTaskWhileOn(const TaskHandle_t &newTaskHandle);
};

//==========================================================>>

/**
 * @brief Implements a Debounced Delayed MPB (**DD-MPB**).
 *
 * The **Debounced Delayed Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released. The reasons to add the delay are design related and are usually used to avoid unintentional presses, or to give some equipment (load) that needs time between repeated activations the benefit of the pause. If the push button is released before the delay configured, no press is registered at all. The delay time in this class as in the other that implement it, might be zero (0), defined by the developer and/or modified in runtime.
 *
 * @note If the **delay** attribute is set to 0, the resulting object is equal to a **DbncdMPBttn** class object.
 *
 * @class DbncdDlydMPBttn
 */
class DbncdDlydMPBttn: public DbncdMPBttn{
protected:
    unsigned long int _strtDelay {0};

    virtual bool updValidPressesStatus();
public:
    /**
     * @brief Default constructor
     *
     */
    DbncdDlydMPBttn();
    /**
     * @brief Class constructor
     *
     * @param strtDelay Sets the initial value for the **strtDelay** attribute.
     *
     * @note For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
     *
     * @note If the **delay** attribute is set to 0, the resulting object is equivalent to a **DbncdMPBttn** class object.
     *
     */
    DbncdDlydMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Class constructor
     *
     * @param strtDelay See DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int)
     *
     * @note For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
     *
     * @note If the **delay** attribute is set to 0, the resulting object is equal to a **DbncdMPBttn** class object.
     *
     */
    DbncdDlydMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Gets the current value of strtDelay attribute.
     *
     * Returns the current value of time used by the object to rise the isOn flag, after the debouncing process ends, in milliseconds. If the MPB is released before completing the debounce **and** the strtDelay time, no press will be detected by the object, and the isOn flag will not be affected. The original value for the delay process used at instantiation time might be changed with the setStrtDelay() method, so this method is provided to get the current value in use.
     *
     * @return The current strtDelay time in milliseconds.
     */
    unsigned long int getStrtDelay();
    /**
     *
     * @brief see DbncdMPBttn::init(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
     *
     */
    bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     *
     * @brief see DbncdMPBttn::init(gpioPinId_t, const bool, const bool, const unsigned long int)
     *
     */
    bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Sets a new value to the **delay** attribute
     *
     * @param newStrtDelay New value for the **delay** attribute.
     *
     * @retval true: the existing delay value was different from newStrDelay, the value was updated to newStrtValue.
     * @retval false: the existing delay value and newStrtDelay were equal no change was made.
     *
     * @note Setting the delay attribute to 0 makes the instantiated object act exactly as a Debounced MPB (D-MPB)
     * @warning: Using a very high **delay** values are valid but might make the system seem less responsive, be aware of how it will affect the user experience.
     */
    bool setStrtDelay(const unsigned long int &newStrtDelay);
};

//==========================================================>>

/**
 * @brief Abstract class, base to implement Latched Debounced Delayed MPBs (**LDD-MPB**).
 *
 * **Latched DD-MPB** are MPBs whose distinctive characteristic is that implement switches that keep the ON state since the moment the input signal is stable (debouncing + Delay process), and keeps the ON state after the MPB is released and until an event un-latches them, setting them free to move to the **Off State**.
 * The un-latching mechanisms include but are not limited to: same MPB presses, timers, other MPB presses, other GPIO external un-latch signals or the use of the public method unlatch().
 * The different un-latching events defines the sub-classes of the LDD-MPB class.
 *
 * @class LtchMPBttn
 */
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
   /**
    * @brief Class constructor
    *
    * @note For the parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
    *
    */
   LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	LtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	/**
	 * @brief See 	DbncdMPBttn::clrStatus(bool)
	 *
	 */
	void clrStatus(bool clrIsOn = true);
   /**
	 * @brief Gets the value of the isLatched flag, indicating the **Latched** or **Unlatched** condition of the object.
	 *
	 * The isLatched flag is automatically updated periodically by the timer that calculates the object state.
	 *
    * @retval true: the object is in **Latched condition**.
    * @retval false: The object is in **Unlatched condition**.
    */
	const bool getIsLatched() const;
	/**
	 * @brief Gets the value of the trnOffASfAP class attribute
	 *
	 * The range of signals accepted by the instantiated objects to execute the unlatch process is diverse, and their nature and characteristics might affect the expected switch behavior. While some of the signals might be instantaneous, meaning that the **start of the unlatch signal** is coincidental with the **end of the unlatch signal**, some others might extend the time between both ends. The **trnOffASAP** flag sets the behavior of the MPB in the second case.
	 * - If the **trnOffASAP** flag is set (true) the **isOn** flag will be reseted as soon as the **start of the unlatch signal** is detected
	 * - If the **trnOffASAP** flag is reset (false) the **isOn** flag will be reseted only when the **end of the unlatch signal** is detected.
	 *
	 * @return The current value of the trnOffASAP attribute.
	 */
	bool getTrnOffASAP();
	/**
	 * @brief Gets the value of the validUnlatchPending attribute
	 *
	 * The validUnlatchPending holds the existence of a still to be processed confirmed unlatch signal. Getting it's current value makes possible taking actions before the unlatch process is started or even discard it completely by using the setUnlatchPend(const bool) method.
	 *
	 * @return The current value of the validUnlatchPending attribute.
	 */
	const bool getUnlatchPend() const;
	/**
	 * @brief See DbncdMPBttn::resetFda()
	 *
	 */
	bool resetFda();
	/**
	 * @brief Sets the value of the trnOffASAP attribute
	 *
	 * @param newVal New value for the trnOffASAP attribute
	 *
	 * @return The value of the trnOffASAP attribute after the method execution
	 */
	bool setTrnOffASAP(const bool &newVal);
	/**
	 * @brief Sets the value of the validUnlatchPending attribute
	 *
	 * By setting the value of the validUnlatchPending it's possible to modify the current MPB status by generating an unlatch signal or by canceling an existent unlatch signal.
	 *
	 * @param newVal New value for the validUnlatchPending attribute
	 *
	 * @return The value of the validUnlatchPending attribute after the method execution
	 */
	bool setUnlatchPend(const bool &newVal);
	/**
	 * @brief Sets the value of the validUnlatchPending attribute
	 *
	 * By setting the value of the validUnlatchPending it's possible to modify the current MPB status by generating an unlatch signal or by canceling an existent unlatch signal.
	 *
	 * @param newVal New value for the validUnlatchPending attribute
	 *
	 * @return The value of the validUnlatchPending attribute after the method execution
	 */
	bool unlatch();
   /**
	 * @brief See DbncdMPBttn::begin(const unsigned long int)
    *
    */
	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

/**
 * @brief Implements a Toggle Latch DD-MPB, a.k.a. a Toggle Switch (**ToLDD-MPB**).
 *
 * The **Toggle switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and keeps the ON state after the push button is released and until it is pressed once again. So this simulates a simple On-Off switch like the ones used to turn on/off a room light. The included methods lets the designer define the unlatch event as the instant de MPB is started to be pressed for the second time or when the MPB is released from that second press.
 *
 * @class TgglLtchMPBttn
 */
class TgglLtchMPBttn: public LtchMPBttn{
protected:
	virtual void updValidUnlatchStatus();
	virtual void stOffNVURP_Do();
public:
	TgglLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	TgglLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
};

//==========================================================>>

/**
 * @brief Implements a Timer Latch DD-MPB, a.k.a. a Timer Switch (**TiLDD-MPB**).
 *
 * The **Timer switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and until the unlatch signal is provided by a preseted timer **started immediately after** the MPB has passed the debounce & delay process.
 * The time count down might be reseted by pressing the MPB before the timer expires by optionally configuring the object to do so with the provided method.
 * The total count down time might be changed by using a provided method.
 *
 * class TmLtchMPBttn
 *
 */
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

/**
 * @brief Implements a Hinted Timer Latch DD-MPB, a.k.a. Staircase Switch (**HTiLDD-MPB**).
 *
 * The **Staircase switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and until the unlatch signal is provided by a preseted timer **started immediately after** the MPB has passed the debounce & delay process.
 * A warning flag might be configured to raise when time to keep the ON signal is close to expiration, based on a configurable % of the total **On State** time.
 * The time count down might be reseted by pressing the MPB before the timer expires by optionally configuring the object to do so with the provided method.
 * A **Pilot Signal** flag is included to emulate completely the staircase switches, that might be activated while the MPB is in **Off state**,  by optionally configuring the object to do so with the provided method. This might be considered just a perk as it's not much more than the **isOn** flag negated output, but gives the advantage of freeing the designer of additional coding.
 *
 * class HntdTmLtchMPBttn
 */
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

/**
 * @brief Implements an External Unlatch LDD-MPB, a.k.a. Emergency Latched Switch (**XULDD-MPB**)
 *
 * The **External released toggle switch** (a.k.a. Emergency latched), keeps the On state since the moment the signal is debounced, and until an external signal is received. This kind of switch is used when an "abnormal situation" demands the push of the switch On, but a higher authority is needed to reset it to Off from a different signal source. The **On State** will then not only start a response to the exception arisen, but will be kept to flag the triggering event.
 *  Smoke, flood, intrusion alarms and "last man locks" are some examples of the use of this switch. As the external release signal can be physical or logical generated it can be implemented to be received from a switch or a remote signal of any usual kind.
 *
 * class XtrnUnltchMPBttn
 */
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

/**
 * @brief Abstract class, base to implement Double Action LDD-MPBs (**DALDD-MPBs**).
 *
 * **Double Action Latched DD-MPB** are MPBs whose distinctive characteristic is that implement switches that present two different behaviors based on the time length of the presses detected.
 * The pattern selected for this class is the following:
 * - A **short press** makes the MPB to behave as a Toggle LDD-MPB Switch (**ToLDD-MPB**) -designated as the **main behavior**-, swapping from the **Off state** to the **On state** and back as usual LDD-MPB.
 * - A **long press** activates another behavior, allowing the only MPB to be used as a second MPB. That different behavior -designated as the **secondary behavior**- defines the sub-classes of the **DALDD-MPB** class.
 * Using a notation where the first component is the Off/On state of the main behavior and the second component the state of the secondary behavior the simplest activation path would be:
 * - 1. Off-Off
 * - 2. On-Off
 * - 3. On-On
 * - 4. On-Off
 * - 5. Off-Off
 *
 * The presses patterns are:
 * - 1. -> 2.: short press.
 * - 1. -> 3.: long press.
 * - 2. -> 3.: long press.
 * - 2. -> 5.: short press.
 * - 3. -> 4.: release.
 * - 4. -> 5.: short press.
 *
 * @note The **short press** will always be calculated as the Debounce + Delay set attributes.
 * @note The **long press** is a configurable attribute of the class, the **Secondary Mode Activation Delay** (scndModActvDly) that holds the time after the Debounce + Delay period that the MPB must remain pressed to activate the mentioned mode.
 *
 * @class DblActnLtchMPBttn
 */
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
	/**
	 * @brief Gets the current value of the  scndModActvDly class attribute.
	 *
	 * The scndModActvDly attribute defines the time length a MPB must remain pressed to consider it a **long press**.
	 *
	 */
   unsigned long getScndModActvDly();
	/**
	 * @brief Sets a new value for the scndModActvDly class attribute
	 *
	 * The scndModActvDly attribute defines the time length a MPB must remain pressed to consider it a **long press**.
	 *
	 * @param newVal The new value for the scndModActvDly attribute.
	 *
	 * @retval true: The new value is different from the existent setting, the value was updated.
	 * @retval false: The new value is equal from the existent setting, no change was made.
	 *
	 */
	bool setScndModActvDly(const unsigned long &newVal);

   bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>
/**
 * @brief Implements a Debounced Delayed DALDD-MPB combo switch, a.k.a. Dobule Debounced Delayed Latched MPB combo switch (**DD-DALDD-MPB**)
 *
 * This is a subclass of the **DALDD-MPB** whose **secondary behavior** is that of a DbncdDlydMPBttn (DD-MPB), that implies that:
 * - While on the 1.state (Off-Off), a short press will activate only the regular **main On state** 2. (On-Off).
 * - While on the 1.state (Off-Off), a long press will activate both the regular **main On state** and the **secondary On state** simultaneously 3. (On-On).
 * When releasing the MPB the switch will stay in the **main On state** 2. (On-Off).
 * While in the 2. state (On-Off), a short press will set the switch to the 5./1. state (Off-Off)
 * While in the 2. state (On-Off), a long press will set the switch to the 3. state (On-On), until the releasing of the MPB, returning the switch to the **main On state** 2. (On-Off).
 *
 * class DDlydDALtchMPBttn
 */
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

/**
 * @brief Implements a Slider Double Action LDD-MPB combo switch, a.k.a. off/on/dimmer, a.k.a. off/on/volume radio switch)(**S-DALDD-MPB**)
 *
 * This is a subclass of the **DALDD-MPB** whose **secondary behavior** is that of a **Digital potentiometer (DigiPot)** or a **Discreet values incremental/decremental register** that means that when in the second mode, while the MPB remains pressed, an attribute set as a register changes its value. The minimum and maximum values, rate in steps/seconds, size of each step and direction (incremental or decremental) are all configurable, as is the starting value and the mechanism to revert direction.
 *
 * class SldrDALtchMPBttn
 */
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
   /**
	 * @brief Class constructor
    *
    * @param initVal (Optional) Initial value of the **wiper**, in this implementation the value corresponds to the **Output Current Value (otpCurVal)** attribute of the class. As the attribute type is uint16_t and the minimum and maximum limits are set to 0x0000 and 0xFFFF respectively, the initial value might be set to any value of the type. If no value is provided 0xFFFF will be the instantiation value.
    *
    * @note For the remaining parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)    */
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

/**
 * @brief Abstract class, base to implement Voidable DD-MPBs (**VDD-MPB**).
 *
 * **Voidable DD-MPBs** are MPBs whose distinctive characteristic is that implement switches that while being pressed their state might change from **On State** to a **Voided & Off state** due to different voiding conditions.
 * Those conditions include but are not limited to: pressed time, external signals or reaching the **On state**.
 * The mechanisms to "un-void" the MPB and return it to an operational state include but are not limited to:
 * - releasing the MPB
 * - receiving an external signal
 * - the reading of the **isOn** flag
 * - others.
 * The voiding condition and the un-voiding mechanism define the VDD-MPB subclasses.
 *
 * @class VdblMPBttn
 */
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

/**
 * @brief Implements a Time Voidable DD-MPB, a.k.a. Anti-tampering switch (**TVDD-MPB**)
 *
 * The **Time Voidable Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released, or until a preset time in the ON state is reached. Then the switch will return to the Off position until the push button is released and pushed back. This kind of switches are used to activate limited resources related management or physical safety devices, and the possibility of a physical blocking of the switch to extend the ON signal forcibly beyond designer's plans is highly undesired. Water valves, door unlocking mechanisms, hands-off security mechanisms, high power heating devices are some of the usual uses for these type of switches.
 *
 * class TmVdblMPBttn
 */
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
