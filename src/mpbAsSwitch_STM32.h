/**
  ******************************************************************************
  * @file	: mpbAsSwitch_STM32.h
  * @brief	: Header file for the MpbAsSwitch_STM32 library classes
  *
  * @details The library builds several switch mechanisms replacements out of simple push buttons
  * or similar equivalent digital signal inputs.
  * By using just a push button (a.k.a. momentary switches or momentary buttons, _**MPB**_
  * for short from here on) the classes implemented in this library will manage,
  * calculate and update different parameters to **generate the embedded behavior of standard
  * electromechanical switches**.
  *
  * @author	: Gabriel D. Goldman
  * @version v2.2.0
  * @date	: Created on: 06/11/2023
  * 			: Last modification:	07/07/2024
  * @copyright GPL-3.0 license
  *
  * @note FreeRTOS Kernel V10.3.1, some implementations have been limited to comply to the services provided by the version.
  ******************************************************************************
  * @attention	This library was developed as part of the refactoring process for an industrial machines security enforcement and control (hardware & firmware update). As such every class included complies **AT LEAST** with the provision of the attributes and methods to make the hardware & firmware replacement transparent to the controlled machines. Generic use attribute and methods were added to extend the usability to other projects and application environments, but no fitness nor completeness of those are given but for the intended refactoring project.
  * **Use of this library is under your own responsibility**
  *
  ******************************************************************************
  */

#ifndef MPBASSWITCH_STM32_H_
#define MPBASSWITCH_STM32_H_

#include <stdint.h>
#include <string>
#include <stdio.h>

//===========================>> Next lines included for developing purposes, corresponding headers must be provided for the production platform/s
#ifndef MCU_SPEC
	#ifndef __STM32F4xx_HAL_H
		#include "stm32f4xx_hal.h"
	#endif

//This depends on the existence of #define HAL_GPIO_MODULE_ENABLED in stm32f4xx_hal_conf.h which is included in stm32f4xx_hal.h
	#ifndef __STM32F4xx_HAL_GPIO_H
		#include "stm32f4xx_hal_gpio.h"
	#endif
#endif
//===========================>> Previous lines included for developing purposes, corresponding headers must be provided for the production platform/s

//===========================>> BEGIN libraries used to avoid CMSIS wrappers
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
//===========================>> END libraries used to avoid CMSIS wrappers

#define _HwMinDbncTime 20  // Documented minimum wait time for a MPB signal to stabilize to consider it pressed or released (in milliseconds)
#define _StdPollDelay 10	// Reasonable time between polls for MPBs switches (in milliseconds)
#define _MinSrvcTime 100	// Minimum valid time value for service/active time for Time Latched MPBs to avoid stability issues relating to debouncing, releasing and other timed events
#define _InvalidPinNum 0xFFFF	// Value to give as "yet to be defined", the "Valid pin number" range and characteristics are development platform and environment dependable

/*---------------- xTaskNotify() mechanism related constants, argument structs, information packing and unpacking BEGIN -------*/
const uint8_t IsOnBitPos {0};
const uint8_t IsEnabledBitPos{1};
const uint8_t PilotOnBitPos{2};
const uint8_t WrnngOnBitPos{3};
const uint8_t IsVoidedBitPos{4};
const uint8_t IsOnScndryBitPos{5};
const uint8_t OtptCurValBitPos{16};

#ifndef MPBOTPTS_T
	#define MPBOTPTS_T
	/**
	 * @brief Type to hold the complete set of output attribute flags from any DbncdMPBttn class and subclasses object.
	 *
	 * Only two members (isOn and isEnabled) are relevant to all classes, the rest of the members might be relevant for one or more of the classes.
	 * The type is provided as a standard return value for the decoding of the 32-bit notification value provided by the use of the xTaskNotify() inter-task mechanism. See setTaskToNotify(const TaskHandle_t) for more information.
	 */
	struct MpbOtpts_t{
		bool isOn;
		bool isEnabled;
		bool pilotOn;
		bool wrnngOn;
		bool isVoided;
		bool isOnScndry;
		uint16_t otptCurVal;
	};
#endif
/*---------------- xTaskNotify() mechanism related constants, argument structs, information packing and unpacking END -------*/

#ifndef GPIOPINID_T
	#define GPIOPINID_T
	/**
	 * @brief Type used to keep GPIO pin identification as a single parameter, independently of the platform requirements.
	 *
	 * GPIO pin identification is hardware and development environment framework dependents, for some platforms it needs one, some two, some more parameters, and each one of these parameters' type depends once again on the platform. This type is provided to define each pin referenced as a single parameter for class methods and attributes declarations, as platform independent as possible.
	 *
	 * @struct gpioPinId_t
	 */
	struct gpioPinId_t{	//
		GPIO_TypeDef* portId;	/**< The port identification as a pointer to a GPIO_TypeDef information structure*/
		uint16_t pinNum;	/**< The number of the port pin represented as a single-bit mask with the set bit position indicating the pin number*/
	};
#endif	//GPIOPINID_T

// Definition workaround to let a function/method return value to be a function pointer
typedef void (*fncPtrType)();
typedef  fncPtrType (*ptrToTrnFnc)();

//===========================>> BEGIN General use function prototypes
uint8_t singleBitPosNum(uint16_t mask);
MpbOtpts_t otptsSttsUnpkg(uint32_t pkgOtpts);
//===========================>> END General use function prototypes

//===========================>> BEGIN General use Global variables
static BaseType_t errorFlag {pdFALSE};
//===========================>> END General use Global variables

//==========================================================>> Classes declarations BEGIN
/**
 * @brief Base class, models a Debounced Momentary Push Button (**D-MPB**).
 *
 * This class provides the resources needed to process a momentary digital input signal -as the one provided by a MPB (Momentary Push Button)- returning a clean signal to be used as a switch, implementing the needed services to replace a wide range of physical related switch characteristics: Debouncing, deglitching, disabling.
 *
 * More physical switch situations can be emulated, like temporarily disconnecting it (isDisabled=true and isOnDisabled=false), short circuiting it (isDisabled=true and isOnDisabled=true) and others.
 *
 * @class DbncdMPBttn
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
   unsigned long int _strtDelay {0};
	bool _sttChng {true};
	TaskHandle_t _taskToNotifyHndl {NULL};
	TaskHandle_t _taskWhileOnHndl{NULL};
	volatile bool _validDisablePend{false};
	volatile bool _validEnablePend{false};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};

	void clrSttChng();
	const bool getIsPressed() const;
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);
   void _setIsEnabled(const bool &newEnabledValue);
	virtual uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
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
	 * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
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
	 * The frequency of the periodic monitoring is passed as a parameter in milliseconds, and is a value that must be small (frequent) enough to keep the object updated, but not so frequent that wastes resources from other tasks. A default value is provided based on empirical results obtained in various published tests.
	 *
	 * @param pollDelayMs (Optional) unsigned long integer (ulong), the time between polls in milliseconds.
	 *
	 * @return Boolean indicating if the object could be attached to a timer.
	 * @retval true: the object could be attached to a timer -or it was already attached to a timer when the method was invoked-.
	 * @retval false: the object could not create the needed timer, or the object could not be attached to it.
	 */
	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief Clears and resets flags, timers and counters modified through the object's signals processing.
	 *
	 * Resets object's attributes to initialization values to safely resume operations, either after pausing the timer, enabling the object after disabling it or any disruptive activity that might generate unexpected distorsions. This avoids risky behavior of the object due to dangling flags or partially ran time counters.
	 *
	 * @param clrIsOn Optional boolean value, indicates if the _isOn flag must be included to be cleared:
	 *
	 * - true (default value) includes the isOn flag.
	 * - false excludes the isOn flag.
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
    */
	void disable();
   /**
	 * @brief Enables the input signal processing.
	 *
	 * Invoking the enable() method on a object in **Disabled state** sends it a message requesting to resume it's normal operation. The execution of the re-enabling of the object implies:
	 * - Resuming all input signals reading.
	 * - Resuming all output flag computation from the "fresh startup" state, including clearing the **isOn state**
	 * - Due to strict security enforcement the object will not be allowed to enter the **Enabled state** if the MPB was pressed when the enable message was received and until a MPB release is efectively detected.
    */
   void enable();
	/**
	 * @brief Detaches the object from the timer that monitors the input pins, compute and updates the object's status. The timer daemon entry is deleted for the object.
	 *
	 * The immediate detachment of the object from the timer that keeps it's state updated implies that the object's state will be kept, whatever that state is it. If a certain status is preferred some of the provided methods should be used for that including clrStatus(), resetFda(), disable(), setIsOnDisabled(), etc. Also consider that if a task is set to be executed while the object is in **On state**, the **end()** invocation wouldn't affect that task execution state.
	 *
	 * @return Boolean indicating the success of the operation
	 * @retval true: the object detachment procedure and timer entry removal was successful.
	 * @retval false: the object detachment and/or entry removal was rejected by the O.S..
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
	 * @brief Gets the function that is set to execute every time the object **enters** the **Off State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOffPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **Off State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **Off State**.
	 *
	 * @warning The function code execution will become part of the list of procedures the object executes when it entering the **Off State**, including the modification of affected attribute flags, suspending the execution of the task running while in **On State** and others. Making the function code too time demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType  getFnWhnTrnOff();
	/**
	 * @brief Gets the function that is set to execute every time the object **enters** the **On State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOnPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **On State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **On State**.
	 *
	 * 	 * @warning The function code execution will become part of the list of procedures the object executes when it entering the **On State**, including the modification of affected attribute flags, suspending the execution of the task running while in **On State** and others. Making the function code too time demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
   fncPtrType getFnWhnTrnOn();
   /**
	 * @brief Gets the value of the isEnabled attribute flag, indicating the **Enabled** or **Disabled** status of the object.
	 *
	 * The isEnabled flag might be modified by the enable() and the disable() methods.
	 *
    * @retval true: the object is in **Enabled state**.
    * @retval false: The object is in **Disabled state**.
    */
   const bool getIsEnabled() const;
   /**
	 * @brief Gets the value of the **isOn** attribute flag.
	 *
	 * The **isOn** attribute flag is the fundamental attribute of the object, it might be considered the "Raison d'etre" of all this classes design: the isOn signal is not just the detection of an expected voltage value at a mcu pin, but the combination of that voltage level, filtered and verified, for a determined period of time and until a new event modifies that situation.  While other mechanism are provided to execute code when the status of the object changes, all but the **isOn** flag value update are optionally executed.
	 *
    * @retval true: The object is in **On state**.
    * @retval false: The object is in **Off state**.
    */
   const bool getIsOn () const;
   /**
	 * @brief Gets the value of the **isOnDisabled** attribute flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**. That might be changed when needed.
	 * In the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this flag's value.
	 * The reasons to disable the ability to change the output, and keep it on either state until re-enabled are design and use dependent.
	 * The flag value might be changed by the use of the setIsOnDisabled() method.
    *
    * @retval true: the object is configured to be set to the **On state** while it is in **Disabled state**.
    * @retval false: the object is configured to be set to the **Off state** while it is in **Disabled state**.
    */
   const bool getIsOnDisabled() const;
   /**
	 * @brief Gets the relevant attribute flags values for the object state encoded as a 32 bits value, required to pass current state of the object to another thread/task managing the outputs
    *
    * The inter-tasks communication mechanisms implemented on the class includes a xTaskNotify() that works as a light-weigh mailbox, unblocking the receiving tasks and sending to it a 32_bit value notification. This function returns the relevant attribute flags values encoded in a 32 bit value, according the provided encoding described.
    *
    * @return A 32-bit unsigned value representing the attribute flags current values.
    */
   const uint32_t getOtptsSttsPkgd();
   /**
	 * @brief Gets the value of the **outputsChange** attribute flag.
	 *
	 * The instantiated objects include attributes linked to their computed state, which represent the behavior expected from their respective electromechanical simulated counterparts.
	 * When any of those attributes values change, the **outputsChange** flag is set. The flag only signals changes have happened -not which flags, nor how many times changes have taken place- since the last **outputsChange** flag reset.
	 * The **outputsChange** flag must be reset (or set if desired) through the setOutputsChange() method.
	 *
    * @retval true: any of the object's behavior flags have changed value since last time **outputsChange** flag was reseted.
    * @retval false: no object's behavior flags have changed value since last time **outputsChange** flag was reseted.
	 */
   const bool getOutputsChange() const;
   /**
    * @brief Gets the current value of strtDelay attribute.
    *
    * Returns the current value of time used by the object to rise the isOn flag, after the debouncing process ends, in milliseconds. If the MPB is released before completing the debounce **and** the strtDelay time, no press will be detected by the object, and the isOn flag will not be affected. The original value for the delay process used at instantiation time might be changed with the setStrtDelay() method, so this method is provided to get the current value in use.
    *
    * @return The current strtDelay time in milliseconds.
    *
    * @attention The strtDelay attribute is forced to a 0 ms value at instantiation of DbncdMPBttn class objects, and no setter mechanism is provided in this class. The inherited DbncdDlydMPBttn class objects (and all it's subclasses) constructor includes a parameter to initialize the strtDelay value, and a method to set that attribute to a new value.
    */
   unsigned long int getStrtDelay();
   /**
	 * @brief Gets the task to be notified by the object when its output flags changes.
	 *
	 * The task handle of the task to be notified by the object when its **outputsChange** attribute flag is set (see getOutputsChange()) holds a **NULL** when the object is created. A valid task's TaskHandle_t value might be set by using the setTaskToNotify() method, and even set back to **NULL** to disable the task notification mechanism.
	 *
    * @return TaskHandle_t value of the task to be notified of the outputs change.
    * @retval NULL: there is no task configured to be notified of the outputs change.
    *
    *@note The notification is done through a **direct to task notification** using the **xTaskNotify()** RTOS macro, the notification includes passing the notified task a 32-bit notification value.
    */
	const TaskHandle_t getTaskToNotify() const;
	/**
	 * @brief Gets the task to be run (resumed) while the object is in the **On state**.
	 *
	 * Gets the task handle of the task to be **resumed** every time the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * @return The TaskHandle_t value of the task to be resumed while the object is in **On state**.
    * @retval NULL if there is no task configured to be resumed while the object is in **On state**.
    *
    * @warning Free-RTOS has no mechanism implemented to notify a task that it is about to be set in **paused** state, so there is no way to that task to ensure it will be set to pause in an orderly fashion. The task to be designated to be used by this mechanism has to be task that can withstand being interrupted at any point of it's execution, and be restarted from that same point next time the **isOn** flag is set. For tasks that might need attaching resources or other issues every time it is resumed and releasing resources of any kind before being **paused**, using the function attached by using **setFnWhnTrnOnPtr()** to gain control of the resources before resuming a task, and the function attached by using **setFnWhnTrnOffPtr()** to release the resources and pause the task in an orderly fashion, or use those functions to manage a binary semaphore for managing the execution of a task.
	 */
	const TaskHandle_t getTaskWhileOn();
	/**
	 * @brief Initializes an object instantiated by the default constructor
	 *
	 * All the parameters correspond to the non-default constructor of the class, DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
	 */
	bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Initializes an object instantiated by the default constructor
	 *
	 * All the parameters correspond to the non-default constructor of the class, DbncdMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int)
	 */
	bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Pauses the software timer updating the computation of the object's internal flags value.
	 *
	 * The immediate stop of the timer that keeps the object's state updated implies that the object's state will be kept, whatever that state is it. The same consideration as the end() method applies referring to options to modify the state in which the object will be while in the **Pause state**.
	 *
	 * @retval true: the object's timer could be stopped by the O.S.(or it was already stopped).
	 * @retval false: the object's timer couldn't be stopped by the O.S..
	 */
	bool pause();
	/**
	 * @brief Resets the debounce process time of the object to the value used at instantiation.
	 *
	 *  The debounce process time used at instantiation might be changed with the setDbncTime() as needed, as many times as needed. This method reverts the value to the instantiation time value. In case the value was not specified at instantiation time the default debounce time value will be used.
	 */
	void resetDbncTime();
	/**
	 * @brief Resets the MPB behavior automaton to it's **Initial** or **Start State**
	 *
	 * This method is provided for security and for error handling purposes, so that in case of unexpected situations detected, the driving **Deterministic Finite Automaton** used to compute the MPB objects states might be reset to it's initial state to safely restart it, usually as part of an **Error Handling** procedure.
	 */
	void resetFda();
	/**
	 * @brief Restarts the software timer updating the calculation of the object internal flags.
	 *
	 *  The timer will stop calling the functions for computing the flags values after calling the **pause()** method and will not be updated until the timer is restarted with this method.
	 *
	 * @retval true: the object's timer could be restarted by the O.S..
	 * @retval false: the object's timer couldn't be restarted by the O.S..
	 *
	 * @warning This method will restart the inactive timer after a **pause()** method. If the object's timer was modified by an **end()* method then a **begin()** method will be needed to restart it's timer.
	 */
	bool resume();
	/**
	 * @brief Sets the debounce time.
	 *
	 *	Sets a new time for the debouncing period. The value must be equal or greater than the minimum empirical value set as a property for all the classes, 20 milliseconds, that value is defined in the _HwMinDbncTime constant. A long debounce time will produce a delay in the press event generation, making it less "responsive".
	 *
	 * @param newDbncTime unsigned long integer, the new debounce value for the object.
	 *
	 * @return	A boolean indicating if the debounce time setting was successful.
	 * @retval true: the new value is in the accepted range, the attribute value is updated.
	 * @retval false: the value was out of the accepted range, no change was made.
	 */
	bool setDbncTime(const unsigned long int &newDbncTime);
	/**
	 * @brief Sets the function that will be called to execute every time the object **enters** the **Off State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOff Function pointer to the function intended to be called when the object **enters** the **Off State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOffPtr(void(*newFnWhnTrnOff)());
	/**
	 * @brief Sets the function that will be called to execute every time the object **enters** the **On State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOn: function pointer to the function intended to be called when the object **enters** the **On State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)());
   /**
	 * @brief Sets the value of the **isOnDisabled** flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**. That might be changed as needed.
	 * While in the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this flag value.
	 *
	 * @note The reasons to disable the ability to change the output, and keep it on either state until re-enabled are design and use dependent, being an obvious one security reasons, disabling the ability of the users to manipulate the switch while keeping the desired **On/Off state**. A simple example would be to keep a light on in a place where a meeting is taking place, disabling the lights switches and keeping the **On State**. Another obvious one would be to keep a machine off while servicing it's internal mechanisms, disabling the possibility of turning it on.
    *
    * @warning If the method is invoked while the object is disabled, and the **isOnDisabled** attribute flag is changed, then the **isOn** attribute flag will have to change accordingly. Changing the **isOn** flag value implies that **all** the implemented mechanisms related to the change of the **isOn** attribute flag value will be invoked.
    */
   void setIsOnDisabled(const bool &newIsOnDisabled);
   /**
	 * @brief Sets the value of the attribute flag indicating if a change took place in any of the output attribute flags (IsOn included).
	 *
	 * The usual path for the **outputsChange** flag is to be set by any method changing an output attribute flag, the callback function signaled to take care of the hardware actions because of this changes clears back **outputsChange** after taking care of them. In the unusual case the developer wants to "intercept" this sequence, this method is provided to set (true) or clear (false) outputsChange value.
    *
    * @param newOutputChange The new value to set the **outputsChange** flag to.
    */
   void setOutputsChange(bool newOutputChange);
   /**
	 * @brief Sets the pointer to the task to be notified by the object when its output attribute flags changes.
	 *
	 * When the object is created, this value is set to **NULL**, and a valid TaskHandle_t value might be set by using this method. The task notifying mechanism will not be used while the task handle keeps the **NULL** value, in which case the solution implementation will have to use any of the other provided mechanisms to test the object status, and act accordingly. After the TaskHandle value is set it might be changed to point to other task. If at the point this method is invoked the attribute holding the pointer was not NULL, the method will suspend the pointed task before proceeding to change the attribute value. The method does not provide any verification mechanism to ensure the passed parameter is a valid task handle nor the state of the task the passed pointer might be.
	 *
    * @param newTaskHandle A valid task handle of an actual existent task/thread running.
    *
    * @note As simple as this mechanism is, it's an un-expensive effective solution in terms of resources involved. As a counterpart it's use must be limited to clearly defined implementations, as no value passing mechanism is provided, so global variables will be the easiest way to go, either for the instantiated MPB, or for the relevant attribute flags values copied to global variables, as the case of **isON**, **isEnabled** and others in the case of more complex subclasses.
    */
	void setTaskToNotify(const TaskHandle_t &newTaskHandle);
	/**
	 * @brief Sets the task to be run while the object is in the **On state**.
	 *
	 * Sets the task handle of the task to be **resumed** when the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * If the existing value for the task handle was not NULL before the invocation, the method will verify the Task Handle was pointing to a deleted or suspended task, in which case will proceed to **suspend** that task before changing the Task Handle to the new provided value.
	 *
	 * Setting the value to NULL will disable the task execution mechanism.
    *
    *@note The method does not implement any task handle validation for the new task handle, a valid handle to a valid task is assumed as parameter.
    *
    * @note Consider the implications of the task that's going to get suspended every time the MPB goes to the **Off state**, so that the the task to be run might be interrupted at any point of its execution. This implies that the task must be designed with that consideration in mind to avoid dangerous situations generated by a task not completely done when suspended.
    *
    * @warning Take special consideration about the implications of the execution **priority** of the task to be executed while the MPB is in **On state** and its relation to the priority of the calling task, as it might affect the normal execution of the application.
	 */
	virtual void setTaskWhileOn(const TaskHandle_t &newTaskHandle);
};

//==========================================================>>

/**
 * @brief Models a Debounced Delayed MPB (**DD-MPB**).
 *
 * The **Debounced Delayed Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released. The reasons to add the delay are design related and are usually used to avoid unintentional presses, or to give some equipment (load) that needs time between repeated activations the benefit of the pause. If the push button is released before the debounce and delay times configured are reached, no press is registered at all. The delay time in this class as in the other that implement it, might be zero (0), defined by the developer and/or modified in runtime.
 *
 * @note If the **delay** attribute is set to 0, the resulting object of this class is equivalent in functionality to a **DbncdMPBttn** class object.
 *
 * @class DbncdDlydMPBttn
 */
class DbncdDlydMPBttn: public DbncdMPBttn{
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
     * @note If the **delay** attribute is set to 0, the resulting object is equivalent in functionality to a **DbncdMPBttn** class object.
     */
    DbncdDlydMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Class constructor
     *
     * @param strtDelay See DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int)
     *
     * @note For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
     *
     * @note If the **delay** attribute is set to 0, the resulting object is equal in functionality to a **DbncdMPBttn** class object.
     */
    DbncdDlydMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     *
     * @brief see DbncdMPBttn::init(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
     */
    bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     *
     * @brief see DbncdMPBttn::init(gpioPinId_t, const bool, const bool, const unsigned long int)
     */
    bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Sets a new value to the **delay** attribute
     *
     * @param newStrtDelay New value for the **delay** attribute in milliseconds.
     *
     * @note Setting the delay attribute to 0 makes the instantiated object act exactly as a Debounced MPB (D-MPB)
     * @warning: Using very high **delay** values is valid but might make the system seem less responsive, be aware of how it will affect the user experience.
     */
    void setStrtDelay(const unsigned long int &newStrtDelay);
};

//==========================================================>>

/**
 * @brief Abstract class, base to model Latched Debounced Delayed MPBs (**LDD-MPB**).
 *
 * **Latched DD-MPB** are MPBs whose distinctive characteristic is that implement switches that keep the ON state since the moment the input signal is stable (debouncing + Delay process), and keeps the ON state after the MPB is released and until an event un-latches them, setting them free to move to the **Off State**.
 * The un-latching mechanisms include but are not limited to: same MPB presses, timers, other MPB presses, other GPIO external un-latch signals or the use of the public method unlatch().
 * The different un-latching events defines the sub-classes of the LDD-MPB class.
 *
 * @attention The range of signals accepted by the instantiated objects to execute the unlatch process is diverse, and their nature and characteristics might affect the expected switch behavior. While some of the signals might be instantaneous, meaning that the **start of the unlatch signal** is coincidental with the **end of the unlatch signal**, some others might extend the time between both ends. To accommodate the logic required by each subclass the **_unlatch_** process is then split into two stages:
 * 1. Validated Unlatch signal (or Validated Unlatch signal start).
 * 2. Validated Unlatch Release signal (or Validated Unlatch signal end).
 * The class provides methods to generate the signals independently of the designated signal sources to modify the instantiated object behavior if needed by the design requirements, Validated Unlatch signal (see LtchMPBttn::setUnlatchPend(const bool), Validated Unlatch Release signal (see LtchMPBttn::setUnlatchRlsPend(const bool), or to **set** both flags to generate an unlatch (see LtchMPBttn::unlatch().
 *
 * @warning Generating the unlatach related flags independently of the unlatch signals provided by the LDD-MPB subclasses might result in unexpected behavior, which might generate the locking of the object with it's unexpected consequences.
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
	volatile bool _validUnlatchPend{false};
	volatile bool _validUnlatchRlsPend{false};

	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
	virtual void updFdaState();
	virtual void updValidUnlatchStatus() = 0;

	virtual void stOffNotVPP_In(){};
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
    */
   LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	/**
	 * @brief Class constructor
	 *
	 * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
	 *
	 * For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
	 */
   LtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
	 * @brief See DbncdMPBttn::begin(const unsigned long int)
    */
	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief See 	DbncdMPBttn::clrStatus(bool)
	 */
	void clrStatus(bool clrIsOn = true);
   /**
	 * @brief Gets the value of the isLatched attribute flag, indicating the **Latched** or **Unlatched** condition of the object.
	 *
	 * The isLatched flag is automatically updated periodically by the timer that calculates the object state.
	 *
    * @retval true: the object is in **Latched condition**.
    * @retval false: The object is in **Unlatched condition**.
    */
	const bool getIsLatched() const;
	/**
	 * @brief Gets the value of the trnOffASfAP class attribute flag.
	 *
	 * As described in the class characteristics the unlatching process comprises two stages, Validated Unlatch Signal and Validates unlatch Release Signal, that might be generated simultaneously or separated in time. The **trnOffASAP** attribute flag sets the behavior of the MPB in the second case.
	 * - If the **trnOffASAP** attribute flag is set (true) the **isOn** flag will be reseted as soon as the **Validated Unlatch Signal** is detected
	 * - If the **trnOffASAP** flag is reset (false) the **isOn** flag will be reseted only when the **Validated Unlatch Release signal** is detected.
	 *
	 * @return The current value of the trnOffASAP attribute flag.
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
	 * @brief Gets the value of the validUnlatchReleasePending attribute
	 *
	 * The validUnlatchReleasePending holds the existence of a still to be processed confirmed unlatch released signal. Getting it's current value makes possible taking actions before the unlatch process ends or even discard it completely by using the setUnlatchRlsPend(const bool) method.
	 *
	 * @return The current value of the validUnlatchReleasePending attribute.
	 */
	const bool getUnlatchRlsPend() const;
	/**
	 * @brief See DbncdMPBttn::resetFda()
	 */
	bool resetFda();
	/**
	 * @brief Sets the value of the trnOffASAP attribute flag.
	 *
	 * @param newVal New value for the trnOffASAP attribute
	 */
	void setTrnOffASAP(const bool &newVal);
	/**
	 * @brief Sets the value of the validUnlatchPending attribute
	 *
	 * By setting the value of the validUnlatchPending it's possible to modify the current MPB status by generating an unlatch signal or by canceling an existent unlatch signal.
	 *
	 * @param newVal New value for the validUnlatchPending attribute
	 */
	void setUnlatchPend(const bool &newVal);
	/**
	 * @brief Sets the value of the validUnlatchRlsPending attribute
	 *
	 * By setting the value of the validUnlatchPending and validUnlatchReleasePending flags it's possible to modify the current MPB status by generating an unlatch signal or by canceling an existent unlatch signal.
	 *
	 * @param newVal New value for the validUnlatchReleasePending attribute
	 */
	void setUnlatchRlsPend(const bool &newVal);
	/**
	 * @brief Sets the values of the flags needed to unlatch a latched MPB
	 *
	 * By setting the values of the validUnlatchPending **and** validUnlatchReleasePending flags it's possible to modify the current MPB status by generating an unlatch signal.
	 *
	 * @retval true the object was latched and the unlatch flags were set.
	 * @retval false the object was not latched, no unlatch flags were set.
	 *
	 * @note Setting the values of the validUnlatchPending and validUnlatchReleasePending flags does not implicate immediate unlatching the MPB but providing the unlatching signals. The unlatching signals will be processed by the MPB according to it's embedded behavioral pattern. For example, the signals will be processed if the MPB is in Enabled state and latched, but will be ignored if the MPB is disabled.
	 */
	bool unlatch();
};

//==========================================================>>

/**
 * @brief Models a Toggle Latch DD-MPB, a.k.a. a Toggle Switch (**ToLDD-MPB**).
 *
 * The **Toggle switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and keeps the ON state after the push button is released and until it is pressed once again. So this simulates a simple On-Off switch like the one used to turn on/off a room light. The included methods lets the designer define the unlatch event as the instant the MPB is started to be pressed for the second time or when the MPB is released from that second press.
 *
 * @class TgglLtchMPBttn
 */
class TgglLtchMPBttn: public LtchMPBttn{
protected:
	virtual void updValidUnlatchStatus();
	virtual void stOffNVURP_Do();
public:
	/**
	 * @brief Class constructor
	 *
    * @note For the parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
	TgglLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
/**
 * @brief Class constructor
 *
 * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
 * For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
 */
	TgglLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
};

//==========================================================>>

/**
 * @brief Models a Timer Latch DD-MPB, a.k.a. Timer Switch (**TiLDD-MPB**).
 *
 * The **Timer switch** keeps the **On state** since the moment the signal is stable (debouncing + delay process), and until the unlatch signal is provided by a preset timer **started immediately after** the MPB has passed the debounce & delay process.
 * The time count down might be reseted by pressing the MPB before the timer expires by optionally configuring the object to do so with the provided method.
 * The total count-down time might be changed by using a provided method.
 *
 * class TmLtchMPBttn
 */
class TmLtchMPBttn: public LtchMPBttn{
protected:
    bool _tmRstbl {true};
    unsigned long int _srvcTime {};
    unsigned long int _srvcTimerStrt{0};

    virtual void stOffNotVPP_Out();
    virtual void stOffVPP_Out();
    virtual void updValidUnlatchStatus();
public:
 	/**
 	 * @brief Class constructor
 	 *
 	 * @param srvcTime The service time (time to keep the **isOn** attribute flag raised).
 	 *
 	 * @note For the other parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
     */
    TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Class constructor
     *
     * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
     * @param srvcTime The service time (time to keep the **isOn** attribute flag raised).
     *
     * @note For the rest of the parameters see DbncdMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int)
     */
    TmLtchMPBttn(gpioPinId_t mpbttnPinStrct, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief see DbncdMPBttn::clrStatus(bool)
     */
    void clrStatus(bool clrIsOn = true);
    /**
     * @brief Gets the configured Service Time.
     *
     * @return The current Service Time setting in milliseconds
     */
    const unsigned long int getSrvcTime() const;
    /**
     * @brief Sets a new value to the Service Time attribute
     *
     * @param newSrvcTime New value for the Service Time attribute
     *
     * @note To ensure a safe and predictable behavior from the instantiated objects a minimum Service Time setting guard is provided, ensuring data and signals processing are completed before unlatching process is enforced by the timer. The guard is set by the defined _MinSrvcTime constant.
     *
     * @retval true if the newSrvcTime parameter is equal to or greater than the minimum setting guard, the new value is set.
     * @retval false The newSrvcTime parameter is less than the minimum setting guard, the srvcTime attribute was not changed.
     */
    bool setSrvcTime(const unsigned long int &newSrvcTime);
    /**
     * @brief Configures the timer for the Service Time to be reseted before it reaches unlatching time.
     *
     * If the isResetable attribute flag is cleared the MPB will return to **Off state** when the Service Time is reached no matter if the MPB was pressed again during the service time period. If the attribute flag is set, pressing the MPB (debounce and delay times enforced) while on the **On state** resets the timer, starting back from 0. The reseting might be repeated as many times as desired.
     *
     * @param newIsRstbl The new setting for the isResetable flag.
     */
    void setTmerRstbl(const bool &newIsRstbl);
};

//==========================================================>>

/**
 * @brief Models a Hinted Timer Latch DD-MPB, a.k.a. Staircase Switch (**HTiLDD-MPB**).
 *
 * The **Staircase switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and until the unlatch signal is provided by a preset timer **started immediately after** the MPB has passed the debounce & delay process.
 * A warning flag might be configured to raise when the time left to keep the ON signal is close to expiration, based on a configurable percentage of the total Service Time.
 * The time count down might be reseted by pressing the MPB before the timer expires by optionally configuring the object to do so with the provided method.
 * A **Pilot Signal** attribute flag is included to emulate completely the staircase switches, that might be activated while the MPB is in **Off state**,  by optionally configuring the object to do so with the provided method. This might be considered just a perk as it's not much more than the **isOn** flag negated output, but gives the advantage of freeing the designer of additional coding.
 *
 * class HntdTmLtchMPBttn
 */
class HntdTmLtchMPBttn: public TmLtchMPBttn{

protected:
	bool _keepPilot{false};
	volatile bool _pilotOn{false};
	unsigned long int _wrnngMs{0};
	volatile bool _wrnngOn {false};
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
	uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
	bool updPilotOn();
	bool updWrnngOn();
public:
	/**
	 * @brief Class constructor
	 *
	 * @param wrnngPrctg Time **before expiration** of service time that the warning flag must be set. The time is expressed as a percentage of the total service time so it's a value in the 0 <= wrnngPrctg <= 100 range.
	 *
	 * For the rest of the parameters see TmLtchMPBttn(GPIO_TypeDef*, const uint16_t, const unsigned long int, const bool, const bool, const unsigned long int, const unsigned long int)
	 */
	HntdTmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &actTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	/**
	 * @brief Class constructor
	 *
	 * @param wrnngPrctg Time **before expiration** of service time that the warning flag must be set. The time is expressed as a percentage of the total service time so it's a value in the 0 <= wrnngPrctg <= 100 range.
	 *
	 * For the rest of the parameters see TmLtchMPBttn(gpioPinId_t, const unsigned long int, const bool, const bool, const unsigned long int, const unsigned long int)
	 */
	HntdTmLtchMPBttn(gpioPinId_t mpbttnPinStrct, const unsigned long int &actTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	/**
	 * @brief See DbncdMPBttn::begin(const unsigned long int)
	 */
	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
   /**
    * @brief see DbncdMPBttn::clrStatus(bool)
    */
	void clrStatus(bool clrIsOn = true);
	/**
	 * @brief Gets the current value of the pilotOn attribute flag.
	 *
	 * The pilotOn flag will be set when the isOn attribute flag is reset (~isOn), while the keepPilot attribute is set. If the keepPilot attribute is false the pilotOn will keep reset independently of the isOn flag value.
	 *
	 * @return The current value of the pilotOn flag
	 * @retval true: the pilotOn flag value is true
	 * @retval false: the pilotOn flag value is false
	 */
	const bool getPilotOn() const;
	/**
	 * @brief Gets the current value of the warningOn attribute flag.
	 *
	 * The warningOn flag will be set when the configured service time (to keep the ON signal set) is close to expiration, based on a configurable percentage of the total Service time.
	 *
	 * @return The current value of the warningOn attribute flag.
	 *
	 * @note As there is no configuration setting to keep the warning flag from working, the way to force the flag to stay set or stay reset is by configuring the accepted limits:
	 * - 0: Will keep the warningOn flag always false (i.e. will turn to true 0 ms before reaching the end of Service Time).
	 * - 100: Will keep the warningOn flag always true (i.e. will turn to true for the 100% of the Service Time).
	 */
	const bool getWrnngOn() const;
	/**
	 * @brief Sets the configuration of the keepPilot service attribute.
	 *
	 * @param newKeepPilot The new setting for the keepPilot service attribute.
	 */
	void setKeepPilot(const bool &newKeepPilot);
	/**
	 * @brief See TmLtchMPBttn::setSrvcTime(const unsigned long int)
	 *
	 * @note As the warningOn attribute flag behavior is based on a percentage of the service time setting, changing the value of that service time implies changing the time amount for the warning signal service, recalculating such time as the set percentage of the new service time.
	 */
	bool setSrvcTime(const unsigned long int &newActTime);
	/**
	 * @brief Sets the value for the percentage of service time for calculating the warningOn flag value.
	 *
	 * The amount of **time before expiration** of service time that the warning flag must be set is defined as a percentage of the total service time so it's a value in the 0 <= wrnngPrctg <= 100 range.
	 *
	 * @param newWrnngPrctg The new percentage of service time value used to calculate the time before service time expiration to set the warningOn flag.
	 *
	 * @return Success changing the percentage to a new value
	 * @retval true the value was within range, the new value is set
	 * @retval false the value was outside range, the value change was dismissed.
	 */
	bool setWrnngPrctg (const unsigned int &newWrnngPrctg);
};

//==========================================================>>

/**
 * @brief Models an External Unlatch LDD-MPB, a.k.a. Emergency Latched Switch (**XULDD-MPB**)
 *
 * The **External released toggle switch** (a.k.a. Emergency latched), keeps the On state since the moment the signal is stable (debounced & delayed), and until an external signal is received. This kind of switch is used when an "abnormal situation" demands the push of the switch On, but a higher authority is needed to reset it to Off from a different signal source. The **On State** will then not only start a response to the exception arisen, but will be kept to flag the triggering event.
 *  Smoke, flood, intrusion alarms and "last man locks" are some examples of the use of this switch. As the external release signal can be physical or logical generated it can be implemented to be received from a switch or a remote signal of any usual kind.
 *
 * class XtrnUnltchMPBttn
 */
class XtrnUnltchMPBttn: public LtchMPBttn{

protected:
    DbncdDlydMPBttn* _unLtchBttn {nullptr};
    bool _xtrnUnltchPRlsCcl {false};

 	virtual void stOffNVURP_Do();
 	virtual void updValidUnlatchStatus();
public:
 	/**
	 * @brief Class constructor
	 *
	 * This class constructor makes specific reference to a source for the unlatch signal by including a parameter referencing an object that implements the needed getIsOn() method to get the external unlatch signal.
	 *
 	 * @param unLtchBttn Pointer to a DbncdDlydMPBttn object that will provide the unlatch source signal through it's **getIsOn()** method.
 	 *
 	 * @warning Referencing a DbncdDlydMPBttn subclass object that keeps the isOn flag set for a preset time period might affect the latching/unlatching process, as this class's objects don't check for the isOn condition of the unlatching object prior to setting it's own isOn flag.
 	 *
 	 * @note For the other parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
 	 *
 	 * @note Other unlatch signal origins might be developed through the unlatch() method provided.
 	 */
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
  	/**
 	 * @brief Class constructor
 	 *
 	 * This class constructor makes specific reference to a source of the unlatch signal by including a parameter referencing an object that implements the needed getIsOn() method to get the external unlatch signal.
 	 *
    * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
  	 * @param unLtchBttn Pointer to a DbncdDlydMPBttn object that will provide the unlatch source signal through it's **getIsOn()** method.
  	 *
  	 * @warning Referencing a DbncdDlydMPBttn subclass object that keeps the isOn flag set for a preset time period might affect the latching/unlatching process, as this class's objects don't check for the isOn condition of the unlatching object prior to setting it's own isOn flag.
  	 *
  	 * @note For the other parameters see DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int)
  	 *
  	 * @note Other unlatch signal origins might be developed through the unlatch() method provided.
  	 */
    XtrnUnltchMPBttn(gpioPinId_t mpbttnPinStrct,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    /**
     * @brief Class constructor
     *
     * This class constructor instantiates an object that relies on the **unlatch()** method invocation to release the latched MPB
     *
     * @note For the parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
     */
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    /**
	  * @brief Class constructor
	  *
	  * This class constructor instantiates an object that relies on the **unlatch()** method invocation to release the latched MPB
	  *
	  * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
	  *
	  * @note For the parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
	  */
    XtrnUnltchMPBttn(gpioPinId_t mpbttnPinStrct,
   		const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);

    /**
     * @brief See DbncdMPBttn::begin(const unsigned long int)
     */
    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
    /**
     * @brief See DbncdMPBttn::clrStatus(bool)
     */
    void clrStatus(bool clrIsOn = true);
};

//==========================================================>>

/**
 * @brief Abstract class, base to model Double Action LDD-MPBs (**DALDD-MPBs**).
 *
 * **Double Action Latched DD-MPB** are MPBs whose distinctive characteristic is that implement switches that present two different behaviors based on the time length of the presses detected.
 * The pattern selected for this class is the following:
 * - A **short press** makes the MPB to behave as a Toggle LDD-MPB Switch (**ToLDD-MPB**) -designated as the **main behavior**-, swapping from the **Off state** to the **On state** and back as usual LDD-MPB.
 * - A **long press** activates another behavior, allowing the only MPB to be used as a second MPB. That different behavior -designated as the **secondary behavior**- defines the sub-classes of the **DALDD-MPB** class.
 * Using a notation where the first component is the Off/On state of the main behavior and the second component the state of the secondary behavior the only possible combinations would be:
 * - 1. Off-Off
 * - 2. On-Off
 * - 3. On-On
 *
 * The presses patterns are:
 * - 1. -> 2.: short press.
 * - 1. -> 3.: long press.
 * - 2. -> 3.: long press.
 * - 2. -> 1.: short press.
 * - 3. -> 2.: secondary behavior unlatch (subclass dependent, maybe release, external unlatch, etc.)
 *
 * @note The **short press** will always be calculated as the Debounce + Delay set attributes.
 * @note The **long press** is a configurable attribute of the class, the **Secondary Mode Activation Delay** (scndModActvDly) that holds the time after the Debounce + Delay period that the MPB must remain pressed to activate the mentioned mode. The same time will be required to keep pressed the MPB while in **Main Behavior** to enter the **Secondary behavior**.
 *
 * @class DblActnLtchMPBttn
 */
class DblActnLtchMPBttn: public LtchMPBttn{
protected:
	enum fdaDALmpbStts{
		stOffNotVPP,
		stOffVPP,
		stOnMPBRlsd,
		//--------
		stOnStrtScndMod,
		stOnScndMod,
		stOnEndScndMod,
		//--------
		stOnTurnOff,
		//--------
		stDisabled
	};
   volatile bool _isOnScndry{false};
	fdaDALmpbStts _mpbFdaState {stOffNotVPP};
	unsigned long _scndModActvDly {2000};
	unsigned long _scndModTmrStrt {0};
	bool _validScndModPend{false};

	void (*_fnWhnTrnOffScndry)() {nullptr};
	void (*_fnWhnTrnOnScndry)() {nullptr};
	TaskHandle_t _taskWhileOnScndryHndl{NULL};

	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
   virtual void stDisabled_In(){};
	virtual void stOnStrtScndMod_In(){};
   virtual void stOnScndMod_Do() = 0;
   virtual void stOnEndScndMod_Out(){};
	virtual void _turnOffScndry();
	virtual void _turnOnScndry();
	virtual void updFdaState();
	virtual bool updValidPressesStatus();
   virtual void updValidUnlatchStatus();

public:
	/**
	 * @brief Abstract Class constructor
	 *
	 * @note For parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
	 */
   DblActnLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	/**
	 * @brief Class constructor
	 *
	 * @note For parameters see DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int)
	 */
   DblActnLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
	 * @brief Virtual destructor
    */
	~DblActnLtchMPBttn();
	/**
	 *
	 * @brief See DbncdMPBttn::begin(const unsigned long int)
	 */
   bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief See DbncddMPBttn::clrStatus(bool)
	 */
   void clrStatus(bool clrIsOn = true);
	/**
	 * @brief Gets the function that is set to execute every time the object **enters** the **Secondary Off State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOffScndryPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **Secondary Off State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **Secondary Off State**.
	 */
	fncPtrType getFnWhnTrnOffScndry();
	/**
	 * @brief Gets the function that is set to execute every time the object **enters** the **Secondary On State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOnScndryPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **Secondary On State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **Secondary On State**.
	 */
	fncPtrType getFnWhnTrnOnScndry();
	/**
	 * @brief Gets the current value of the scndModActvDly class attribute.
	 *
	 * The scndModActvDly attribute defines the time length a MPB must remain pressed to consider it a **long press**, needed to activate the **secondary mode**.
	 *
	 * @return The current scndModActvDly value, i.e. the delay in milliseconds.
	 */
   unsigned long getScndModActvDly();
	/**
	 * @brief Gets the task to be run while the object is in the **Secondary On state**.
	 *
	 * Gets the task handle of the task to be **resumed** every time the object enters the **Secondary On state**, and will be **paused** when the  object enters the **Secondary Off state**. This task execution mechanism dependent of the **Secondary On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * @return The TaskHandle_t value of the task to be resumed while the object is in **Secondary On state**.
    * @retval NULL if there is no task configured to be resumed while the object is in **Secondary On state**.
    *
    * @warning Free-RTOS has no mechanism implemented to notify a task that it is about to be set in **paused** state, so there is no way to that task to ensure it will be set to pause in an orderly fashion. The task to be designated to be used by this mechanism has to be task that can withstand being interrupted at any point of it's execution, and be restarted from that same point next time the **isOnScndry** flag is set. For tasks that might need attaching resources or other issues every time it is resumed and releasing resources of any kind before being **paused**, using the function attached by using **setFnWhnTrnOnScndryPtr()** to gain control of the resources before resuming a task, and the function attached by using **setFnWhnTrnOffScndryPtr()** to release the resources and pause the task in an orderly fashion, or use those functions to manage a binary semaphore for managing the execution of a task.
	 */
	const TaskHandle_t getTaskWhileOnScndry();
	/**
	 * @brief Sets the function that will be called to execute every time the object **enters** the **Secondary Off State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOff Function pointer to the function intended to be called when the object **enters** the **Secondary Off State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOffScndryPtr(void(*newFnWhnTrnOff)());
	/**
	 * @brief Sets the function that will be called to execute every time the object **enters** the **Secondary On State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOn: function pointer to the function intended to be called when the object **enters** the **Secondary On State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOnScndryPtr(void (*newFnWhnTrnOn)());
	/**
	 * @brief Sets a new value for the scndModActvDly class attribute
	 *
	 * The scndModActvDly attribute defines the time length a MPB must remain pressed after the end of the debounce&delay period to consider it a **long press**, needed to activate the **secondary mode**. The value setting must be newVal >= _MinSrvcTime to ensure correct signal processing. See TmLtchMPBttn::setSrvcTime(const unsigned long int) for details.
	 *
	 * @param newVal The new value for the scndModActvDly attribute.
	 *
	 * @retval true: The new value is in the valid range, the value was updated.
	 * @retval false: The new value is not in the valid range, the value was not updated.
	 */
	bool setScndModActvDly(const unsigned long &newVal);
	/**
	 * @brief Sets the task to be run while the object is in the **On state**.
	 *
	 * Sets the task handle of the task to be **resumed** when the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * If the existing value for the task handle was not NULL before the invocation, the method will verify the Task Handle was pointing to a deleted or suspended task, in which case will proceed to **suspend** that task before changing the Task Handle to the new provided value.
	 *
	 * Setting the value to NULL will disable the task execution mechanism.
	 *
	 * @retval true: the task indicated by the parameter was successfuly asigned to be executed while in isOn state, or the mechanism was deactivated by setting the task handle to NULL.
    *
    *@note The method does not implement any task handle validation for the new task handle, a valid handle to a valid task is assumed as parameter.
    *
    * @note Consider the implications of the task that's going to get suspended every time the MPB goes to the **Off state**, so that the the task to be run might be interrupted at any point of its execution. This implies that the task must be designed with that consideration in mind to avoid dangerous situations generated by a task not completely done when suspended.
    *
    * @warning Take special consideration about the implications of the execution **priority** of the task to be executed while the MPB is in **On state** and its relation to the priority of the calling task, as it might affect the normal execution of the application.
	 */
	void setTaskWhileOnScndry(const TaskHandle_t &newTaskHandle);

};

//==========================================================>>

/**
 * @brief Models a Debounced Delayed Double Action Latched MPB combo switch (Debounced Delayed DALDD-MPB - **DD-DALDD-MPB**)
 *
 * This is a subclass of the **DALDD-MPB** whose **secondary behavior** is that of a DbncdDlydMPBttn (DD-MPB), that implies that:
 * - While on the 1.state (Off-Off), a short press will activate only the regular **main On state** 2. (On-Off).
 * - While on the 1.state (Off-Off), a long press will activate both the regular **main On state** and the **secondary On state** simultaneously 3. (On-On).
 * When releasing the MPB the switch will stay in the **main On state** 2. (On-Off).
 * While in the 2. state (On-Off), a short press will set the switch to the 1. state (Off-Off)
 * While in the 2. state (On-Off), a long press will set the switch to the 3. state (On-On), until the releasing of the MPB, returning the switch to the **main On state** 2. (On-Off).
 *
 * class DDlydDALtchMPBttn
 */
class DDlydDALtchMPBttn: public DblActnLtchMPBttn{
protected:
	uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
   virtual void stDisabled_In();
   virtual void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do();
   virtual void stOnEndScndMod_Out();
public:
   /**
	 * @brief Class constructor
    *
	 * @note For parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
   DDlydDALtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
	 * @brief Class constructor
    *
	 * @note For parameters see DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
   DDlydDALtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
	 * @brief Class virtual destructor
    */
   ~DDlydDALtchMPBttn();
	/**
	 * @brief See DbncddMPBttn::clrStatus(bool)
	 */
   void clrStatus(bool clrIsOn = true);
   /**
	 * @brief Gets the current value of the isOnScndry attribute flag
	 *
	 * The isOnScndry attribute flag holds the **secondary On state**, when the MPB is pressed for the seted **long press** time from the Off-Off or the On-Off state as described in the DblActnLtchMPBttn class.
    *
    * @return The current value of the isOnScndry flag.
    */
   bool getIsOnScndry();
};

//==========================================================>>

/**
 * @brief Models a Slider Double Action LDD-MPB combo switch, a.k.a. off/on/dimmer, a.k.a. off/on/volume radio switch)(**S-DALDD-MPB**)
 *
 * This is a subclass of the **DALDD-MPB** whose **secondary behavior** is analog to that of a **Digital potentiometer (DigiPot)** or a **Discreet values increment/decrement register**. That means that when in the second mode, while the MPB remains pressed, an attribute set as a register changes its value -the **otptCurVal** register-.
 * When the timer callback function used to keep the MPB status updated is called -while in the secondary mode state- the time since the last call is calculated and the time lapse in milliseconds is converted into **Steps**, using as configurable factor the **outputSliderSpeed** in a pre-scaler fashion. At instantiation the **outputSliderSpeed** is configured to 1 (step/millisecond, i.e. 1 step for each millisecond).
 * The resulting value in "steps" is then factored by the **outputSliderStepSize**, which holds the value that each step will  modify the **otptCurVal** register.
 * The implemented embedded behavior mechanisms of the class determine how the modification of the otpCurVal register will be made, and the associated effects to the instantiated object's attribute, such as (but not limited to):
 * - Incrementing otpCurVal register (by the quantity of steps multiplied by the step size) up to the maximum value setting.
 * - Decrementing otpCurVal register (by that quantity) down to the minimum value setting.
 * - Changing the modification's direction (from incrementing to decrementing or vice versa).
 * The minimum and maximum values, the rate in steps/millisecond, the size of each step and the variation direction (sign of the variation, incrementing or decrementing) are all configurable, as is the starting value and the mechanism to revert the "direction" that includes:
 * - Revert directions in the next **secondary mode** entry.
 * - Automatically revert direction when reaching the minimum and maximum values setting.
 * - Revert direction by methods invocation (see setSldrDirDn(), setSldrDirUp(), swapSldrDir()).
 *
 * class SldrDALtchMPBttn
 */
class SldrDALtchMPBttn: public DblActnLtchMPBttn{

protected:
	bool _autoSwpDirOnEnd{true};	// Changes slider direction automatically when reaches _otptValMax or _otptValMin
	bool _autoSwpDirOnPrss{false};// Changes slider direction each time it enters slider mode
	bool _curSldrDirUp{true};
	uint16_t _initOtptCurVal{};
	uint16_t _otptCurVal{};
	unsigned long _otptSldrSpd{1};
	uint16_t _otptSldrStpSize{0x01};
	uint16_t _otptValMax{0xFFFF};
	uint16_t _otptValMin{0x00};

	uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
	void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do();
	bool _setSldrDir(const bool &newVal);
public:
   /**
	 * @brief Class constructor
    *
    * @param initVal (Optional) Initial value of the **wiper** (taking the analogy of a potentiometer working parts), in this implementation the value corresponds to the **Output Current Value (otpCurVal)** attribute of the class. As the attribute type is uint16_t and the minimum and maximum limits are set to 0x0000 and 0xFFFF respectively, the initial value might be set to any value of the type. If no value is provided 0xFFFF will be the instantiation value.
    *
    * @note For the remaining parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
	SldrDALtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
	/**
	 * @brief Class constructor
	 *
    * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
	 *
	 * @note For the remaining parameters and considerations see SldrDALtchMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int, const uint16_t)
	 */
	SldrDALtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
   /**
	 * @brief Virtual class destructor
    */
	~SldrDALtchMPBttn();
	/**
	 *
	 * @note See DbncdMPBttn::clrStatus(bool)
	 */
   void clrStatus(bool clrIsOn = true);
	/**
	 * @brief Gets the **Output Current Value (otpCurVal)** attribute
	 *
	 * @return The otpCurVal register value.
	 */
   uint16_t getOtptCurVal();
   /**
	 * @brief Gets the result of comparing the **Output Current Value** to the **Maximum value setting**
    *
    * @return The logical result of the comparison
    * @retval true: The **Output Current Value** is equal to the **Maximum value setting**, i.e. the otpCurVal has reached the "top" of the configured range of accepted values.
    * @retval false: The **Output Current Value** is **not** equal to the **Maximum value setting**.
    */
   bool getOtptCurValIsMax();
   /**
	 * @brief Gets the result of comparing the **Output Current Value** to the **Minimum value setting**
    *
    * @return The logical result of the comparison
    * @retval true: The **Output Current Value** is equal to the **Minimum value setting**, i.e. the otpCurVal has reached the "bottom" of the configured range of accepted values.
    * @retval false: The **Output Current Value** is **not** equal to the **Minimum value setting**.
    */
   bool getOtptCurValIsMin();
	/**
	 * @brief Gets the top **output current value** register setting
	 *
	 * @return The maximum **output current value** set.
	 */
   uint16_t getOtptValMax();
	/**
	 * @brief Gets the bottom **output current value** register setting
	 *
	 * @return The minimum **output current value** set.
	 */
	uint16_t getOtptValMin();
	/**
	 * @brief Gets the current setting for the **Output Slider Speed** value.
	 *
	 * The **outputSliderSpeed** attribute is the configurable factor used to convert the time passed since the MPB entered it's secondary mode in milliseconds into **Steps** -Slider mode steps- in a pre-scaler fashion.
	 *
	 * @return The outputSliderSpeed attribute value.
	 *
	 * @note At instantiation the **outputSliderSpeed** is configured to 1 (step/millisecond).
	 */
	unsigned long getOtptSldrSpd();
	/**
	 * @brief Gets the current setting for the **Output Slider Step Size** value.
	 *
	 * The **outputSliderStepSize** is the factor by which the change in steps is multiplied to calculate the total modification of the **otpCurVal** register. As the steps modification is calculated each time the timer callback function is called the variation is done in successive steps while the MPB is kept pressed, and not just when it is finally released.
	 *
	 * @return The outputSliderStepSize attribute value.
	 *
	 * @note At instantiation the **outputSliderStepSize** is configured to 1 (counts/step).
	 */
	uint16_t getOtptSldrStpSize();
	/**
	 * @brief Gets the value of the curSldrDirUp attribute
	 *
	 * The curSldrDirUp attribute indicates the direction at which the outputCurrentValue register is being modified. If the current slider direction is up, means the change of value must be treated as an increment, while having the current slider direction down means it's value must be treated as a decrement.
	 *
	 * @return The current slider direction value
	 * @retval true The current slider direction is **Up**, the output current value will be incremented.
	 * @retval false The current slider direction is **Down**, the output current value will be decremented.
	 */
	bool getSldrDirUp();
	/**
	 * @brief Sets the output current value register.
	 *
	 * The new value for the output current value register must be in the range otptValMin <= newVal <= otptValMax
	 *
	 * @param newVal The new value for the output current value register.
	 *
	 * @return The success in the value change
	 * @retval true The new value was within valid range, the output current value register is set to the parameter passed.
	 * @retval false The new value was outside valid range, the change was not made.
	 */
	bool setOtptCurVal(const uint16_t &newVal);
	/**
	 * @brief Sets the output slider speed (**otptSldrSpd**) attribute.
	 *
	 * As described in the SldrDALtchMPBttn class definition, the **otptSldrSpd** value is the factor by which the time between two readings of the MPB pressing state in secondary mode is converted into "slider steps". At instantiation the value is set to 1, meaning 1 millisecond is equivalent to 1 step, the "**slowest**" speed configuration. Rising the value will make each millisecond represent more steps, making the change of current value faster.
	 *
	 * @param newVal The new value for the **otptSldrSpd** attribute.
	 *
	 * @return The success in the value of the attribute
	 * @retval true The parameter was a valid value, the attribute is set to the new value.
	 * @retval false The parameter is an invalid value, the attribute is not changed.
	 *
	 * @note Making the **otptSldrSpd** equal to 0 would void the ability to change the slider value, so the range of acceptable values is 1<= newVal <= (2^16-1).
	 * @warning A "wrong" combination of **otptSldrSpd** and **otptSldrStpSize** might result in value change between readings greater than the range set by the **otptValMin** and the **otptValMax** values. The relation between the four parameters must be kept in mind if the application developed gives the final user the capability to configure those values at runtime.
	 */
	bool setOtptSldrSpd(const uint16_t &newVal);
	/**
	 * @brief Sets the output slider step size (**otptSldrStpSize**) attribute value.
	 *
	 * @param newVal The new value for the outputSliderStepSize attribute.
	 *
	 * @note The new value for the step size must be smaller or equal than the size of the valid range of otptCurVal attribute
	 *
	 * @retval true If newVal <= (otptValMax - otptValMin), the value of the outputSliderStepSize attribute is changed.
	 * @retval false Otherwise, and the value of the outputSliderStepSize attribute is not changed.
	 *
	 * @note For the secondary function to work like a slider, the condition (newVal < ((_otptValMax - _otptValMin) / _otptSldrSpd)) must be kept
	 * @note For the secondary function to work like a slider or a toggle switch, the condition (newVal <= ((_otptValMax - _otptValMin) / _otptSldrSpd)) must be kept
	 * @note For the secondary function to work like a toggle switch, the condition (newVal = ((_otptValMax - _otptValMin) / _otptSldrSpd)) must be kept.
	 *
	 * @warning As a consequence of the aforementioned notes, if both **otptSldrStpSize** and **otptSldrSpd** are being changed, the **otptSldrSpd** must be changed first to ensure the new **otptSldrStpSize** doesn't fail the range validation test included in this method.
	 */
	bool setOtptSldrStpSize(const uint16_t &newVal);
	/**
	 * @brief Sets the output current value register maximum value attribute (otptValMax attribute).
	 *
	 * The new value for the otptValMax attribute must be in the range otptValMin < newVal <= 0xFFFF
	 *
	 * @param newVal The new value for the otptValMax attribute.
	 *
	 * @return The success in the value change
	 * @retval true The new value was within valid range, the otptValMax attribute change was made.
	 * @retval false The new value was outside valid range, the change was not made.
	 *
	 * @note Due to type constrains, the failure in the value change must be consequence of the selected newVal <= otptValMin
	 *
	 * @warning If the otpValMax attribute intended change is to a smaller value, the otpCurVal might be left outside the new valid range (newVal < otpCurVal). In this case the otptCurVal will be changed to be equal to newVal, and so otptCurVal will become equal to otptValMax.
	 */
	bool setOtptValMax(const uint16_t &newVal);
	/**
	 * @brief Sets the output current value register minimum value attribute (otptValMin attribute).
	 *
	 * The new value for the otptValMin attribute must be in the range 0x0000<= newVal < otptValMax.
	 *
	 * @param newVal The new value for the otptValMin attribute.
	 *
	 * @return The success in the value change
	 * @retval true The new value was within valid range, the otptValMin attribute change was made.
	 * @retval false The new value was outside valid range, the change was not made.
	 *
	 * @note Due to type constrains, the failure in the value change must be consequence of the selected newVal >= otptValMax
	 *
	 * @warning If the otptValMin attribute intended change is to a greater value, the otptCurVal might be left outside the new valid range (newVal > otptCurVal). In this case the otptCurVal will be changed to be equal to newVal, and so otptCurVal will become equal to otptValMin.
	 */
	bool setOtptValMin(const uint16_t &newVal);
	/**
	 * @brief Sets the value of the curSldrDirUp attribute to false.
	 *
	 * The curSldrDirUp attribute dictates how the calculated output value change -resulting from the time lapse between checks, the otptSldrSpd and the otptSldrStpSize attributes- will be applied to the otptCurVal attribute.
	 * - If curSldrDirUp = false, the output value change will be **subtracted** from otptCurVal, successively down to otptValMin.
	 *
	 * @note If the method intends to set the curSldrDirUp to false when the otptCurVal = otptValMin the method will fail, returning false as result.
	 *
	 * @return The success in changing the slider direction
	 * @retval true The change of direction was successful
	 * @retval true The change of direction failed as the otptCurVal was equal to the extreme value
	 */
	bool setSldrDirDn();
	/**
	 * @brief Sets the value of the curSldrDirUp attribute to true
	 *
	 * The curSldrDirUp attribute dictates how the calculated output value change -resulting from the time lapse between checks, the otptSldrSpd and the otptSldrStpSize attributes- will be applied to the otptCurVal attribute.
	 * - If curSldrDirUp = true, the output value change will be **added** to otptCurVal, up to otptValMax.
	 *
	 * @note If the method intends to set curSldrDirUp to true when the otptCurVal = otptValMax the method will fail, returning false as result.
	 *
	 * @return The success in changing the slider direction
	 * @retval true The change of direction was successful
	 * @retval true The change of direction failed as the otptCurVal was equal to the extreme value
	 */
	bool setSldrDirUp();
	/**
	 * @brief Sets the value of the "Auto-Swap Direction on Ends" (**autoSwpDirOnEnd**) attribute.
	 *
	 * The autoSwpDirOnEnd is one of the attributes that configures the slider behavior, setting what the MPB must do when reaching one of the output values range limits.
	 * If a limit -otptValMin or otptValMax- is reached while the MPBttn is kept pressed and in secondary mode, the otptCurVal register increment/decrement can be stopped until a change of direction is invoked through setSldrDirUp(), setSldrDirDn() or swpSldrDir() methods, or automatically through the "Auto-Swap Direction on Ends" (**autoSwpDirOnEnd**) and the "Swap Direction on MPB press" (**autoSwpDirOnPrss**).
	 * If the **autoSwpDirOnEnd** attribute is set (true) the increment direction of the otptCurVal will be automatically inverted when it reaches otptValMax or otptValMin. If it's not set the otptCurVal will not change value until one of the described alternative methods is invoked, or the MPB is released and pushed back if the **autoSwpDirOnPrss** is set -see **setSwpDirOnPrss(const bool)**.
	 *
	 * @param newVal The new value for the autoSwpDirOnEnd attribute
	 */
	void setSwpDirOnEnd(const bool &newVal);
	/**
	 * @brief Sets the value of the "Auto-Swap Direction On Press" (**autoSwpDirOnPrss**) attribute.
	 *
	 * The **autoSwpDirOnPrss** is one of the attributes that configures the slider behavior, setting what the MPB must do when the MPB object enters in secondary mode, referring to the increment or decrement directions.
	 * If the **autoSwpDirOnPrss** attribute is set (true) the increment direction of the otptCurVal will be automatically inverted every time the MPB is pressed back into secondary mode. If the otptCurVal was incrementing when the MPB was last released. the otptCurVal will start decrementing the next time is pressed to enter secondary mode, and vice versa. If it's not set the otptCurVal will not change direction until it reaches one the limit values, or until one of the described alternative methods is invoked, or the MPB's **autoSwpDirOnEnd** attribute is set -see **setSwpDirOnEnd(const bool)** for more details.
	 *
	 * @param newVal The new value for the autoSwpDirOnPrss attribute
	 */
	void setSwpDirOnPrss(const bool &newVal);
	/**
	 * @brief Inverts the current **curSldrDirUp** attribute value.
	 *
	 * Whatever the previous value of the **curSldrDirUp** flag, the method invocation inverts it's value, and as a consequence the inverts the direction of the otptCurVal update. If it was incrementing it will start decrementing, and vice versa, considering all the other factors, attributes and attribute flags involved in the embedded object behavior (minimum and maximum settings, the direction changes if pressed, reaches limits, etc.)
	 *
	 * @return The new value of the curSldrDirUp attribute.
	 */
	bool swapSldrDir();
};

//==========================================================>>

/**
 * @brief Abstract class, base to model Voidable DD-MPBs (**VDD-MPB**).
 *
 * **Voidable DD-MPBs** are MPBs whose distinctive characteristic is that implement non-latching switches that while being pressed their state might change from **On State** to a **Voided state** due to different voiding conditions. Depending on the classes the voided state might be **Voided & Off state**, **Voided & On state** or **Voided & Not enforced** states.
 * Those conditions to change to a voided state include -but are not limited to- the following conditions:
 * - pressing time
 * - external signals
 * - entering the **On state**
 *
 * The mechanisms to "un-void" the MPB and return it to an operational state include -but are not limited to- the following actions:
 * - releasing the MPBs
 * - receiving an external signal
 * - the reading of the **isOn** attribute flag status
 *
 * The voiding conditions and the un-voiding mechanisms define the VDD-MPB subclasses.
 *
 * @class VdblMPBttn
 */
class VdblMPBttn: public DbncdDlydMPBttn{
private:
   void setFrcdOtptWhnVdd(const bool &newVal);
   void setStOnWhnOtpFrcd(const bool &newVal);
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

 	 bool _frcOtptLvlWhnVdd {true};
 	 bool _isVoided{false};
    bool _stOnWhnOtptFrcd{false};
    bool _validVoidPend{false};
    bool _validUnvoidPend{false};

    static void mpbPollCallback(TimerHandle_t mpbTmrCb);
    uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
    bool setVoided(const bool &newVoidValue);
    virtual void stDisabled_In();
    virtual void stDisabled_Out();
    virtual void stOffNotVPP_In(){};
    virtual void stOffVPP_Do(){};	// This provides a setting point for the voiding mechanism to be started
    virtual void stOffVddNVUP_Do(){};	//This provides a setting point for calculating the _validUnvoidPend
    virtual void updFdaState();
    virtual bool updVoidStatus() = 0;
public:
    /**
     * @brief Class constructor
     *
     * @param isOnDisabled (Optional) Sets the instantiation value for the isOnDisabled flag attribute.
     *
     * @note For the other parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
     */
    VdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    /**
     * @brief Class constructor
     *
     * @param mpbttnPinStrct GPIO port and Pin identification defined as a single gpioPinId_t parameter.
     * @param isOnDisabled (Optional) Sets the instantiation value for the isOnDisabled flag attribute.
     *
     * @note For the other parameters see DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int).
     */
    VdblMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    /**
     * @brief Default virtual destructor
     */
    virtual ~VdblMPBttn();
    /**
     * @brief See DbncdMPBttn::clrStatus(bool)
     */
    void clrStatus(bool clrIsOn = true);
    /**
     * @brief Gets the current value of the isVoided attribute flag
     *
     * @return The value of the flag.
     * @retval true The object is in **voided state**
     * @retval false The object is in **not voided state**
     */
    const bool getIsVoided() const;
    /**
     * @brief Sets the value of the isVoided attribute flag to false
     *
     * @warning The value of the isVoided attribute flag is computed as a result of the current state of the instantiated object, considering the inputs and embedded simulated behavior.
     * - Arbitrarily setting a value to the isVoided attribute flag might affect the normal behavior path for the object.
     * - The attribute flag value might return to it's natural value when the behavior imposes the change.
     * - The use of this method must be limited to certain states and conditions of the object, being the most suitable situation while the object is in **Disabled state**: If the application development requires the isVoided attribute flag to be in a specific value, this method and the setIsVoided() method are the required tools.
     *
     * @retval true
     */
    bool setIsNotVoided();
    /**
     * @brief Sets the value of the isVoided attribute flag to true.
     *
     * @warning See the Warnings for setIsNotVoided()
     *
     * @retval true
     */
    bool setIsVoided();
    /**
     * @brief Gets the value of the frcOtptLvlWhnVdd attribute.
     *
     * The frcOtptLvlWhnVdd (Force Output Level When Voided) attribute configures the object to either keep it's isOn attribute flag current value when entering the **voided state** (false) or to force it to a specific isOn value (true).
     *
     * @return the current value of the frcOtptLvlWhnVdd attribute.
     *
     * @note Until v2.0.0 no VdblMPBttn class or subclasses **make use of the frcOtptLvlWhnVdd attribute**, their inclusion is "New Features Requests" reception related.
     */
    bool getFrcOtptLvldWhnVdd();
    /**
     * @brief Gets the value of the stOnWhnOtptFrcd attribute.
     *
     * If the frcOtptLvlWhnVdd attribute flag is set to true, the instantiated object will force the isOn flag attribute value when entering the **voided state**, in which case the enforced value will be defined by the stOnWhnOtptFrcd (Set On When Output Forced) attribute. If this attribute value is true, the isOn attribute flag will be forced to true, if it is false the isOn  attribute flag will be forced to false. In both cases, if the isOn value is forced to change, the respective **turning on** or **turning off** actions will be executed.
     *
     * @return the current value of the stOnWhnOtptFrcd attribute.
     *
     * @note Until v2.0.0 no VdblMPBttn class or subclasses **make use of the stOnWhnOtptFrcd attribute**, their inclusion is "New Features Requests" reception related.
     */
    bool getStOnWhnOtpFrcd();
};

//==========================================================>>

/**
 * @brief Models a Time Voidable DD-MPB, a.k.a. Anti-tampering switch (**TVDD-MPB**)
 *
 * The **Time Voidable Momentary Push Button** keeps the **On state** since the moment the signal is stable (debounce & delay process) and until the moment the push button is released, or until a preset time in the **On state** is reached. Then the switch will return to the Off position until the push button is released and pushed back.
 * This kind of switches are used to activate limited resources related management or physical safety devices, and the possibility of a physical blocking of the switch to extend the ON signal forcibly beyond designer's plans is highly undesired. Water valves, door unlocking mechanisms, hands-off security mechanisms, high power heating devices are some of the usual uses for these type of switches.
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
    /**
     * @brief Class constructor
     *
     * @param voidTime The time -in milliseconds- the MPB must be pressed to enter the **voided state**.
     *
     * @note For the rest of the parameters see VdblMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool , const unsigned long int, const unsigned long int, const bool)
     */
    TmVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    /**
     * @brief Class constructor
     *
     * @param voidTime The time -in milliseconds- the MPB must be pressed to enter the **voided state**.
     *
     * @note For the rest of the parameters see VdblMPBttn(gpioPinId_t, const bool, const bool , const unsigned long int, const unsigned long int, const bool)
     */
    TmVdblMPBttn(gpioPinId_t mpbttnPinStrct, unsigned long int voidTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    /**
     * @brief Class virtual destructor
     */
    virtual ~TmVdblMPBttn();
    /**
     * @brief See DbncdMPBttn::clrStatus(bool)
     */
    void clrStatus();
    /**
     * @brief Gets the voidTime attribute current value.
     *
     * The voidTime attribute holds the time -in milliseconds- the MPB must be pressed to enter the **voided state**.
     *
     * @return The current value of the voidTime attribute.
     */
    const unsigned long int getVoidTime() const;
    /**
     * @brief Sets a new value to the Void Time attribute
     *
     * @param newVoidTime New value for the Void Time attribute
     *
     * @note To ensure a safe and predictable behavior from the instantiated objects a minimum Void Time (equal to the minimum Service Time) setting guard is provided, ensuring data and signals processing are completed before voiding process is enforced by the timer. The guard is set by the defined _MinSrvcTime constant.
     *
     * @retval: true if the newVoidTime parameter is equal to or greater than the minimum setting guard. The attribute value is changed.
     * @retval: false otherwise. The attribute value is not changed.
     */
    bool setVoidTime(const unsigned long int &newVoidTime);
    /**
     * @brief See DbncdMPBttn::begin(const unsigned long int)
     */
    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

/**
 * @brief Models a Single Service Voidable DD-MPB a.k.a. Trigger switch (**SSVDD-MPB**)
 *
 * The **Single Service Voidable Momentary Push Button** keeps the **On state** since the moment the signal is stable (debounce & delay process) and until the moment the provided mechanisms implemented to be executed when the switch enters the **On State** are started, that means calling the **fnWhnTrnOn** function, notifying the **taskToNotify** task and setting the **isOn** attribute flag.
 * After the configured mechanisms are triggered and the attribute flag is set to **true** (the only mandatory action is the attribute flag setting, all the others are configurable to execute or not) the MPB will enter the **Voided State**, forcing the MPB into the **Off State**. The SnglSrvcVdblMPBttn class objects requires the MPB to be released to exit the **Voided State**, restarting the cycle.
 * This kind of switches are used to handle "Single Shot Trigger" style signals, ensuring one single signal per push.
 *
 * @attention Depending on checking the **isOn** flag reading trough the getIsOn() method might surely fail due to the high risk of missing the short time the flag will be raised before is again taken down by the voidance of the MPB. The use of the non-polling facilities ensures no loss of signals and enough time to execute the code depending on the "trigger activation", including the **fnWhnTrnOn** function, and the **taskToNotify** task.
 *
 * @note Due to the short time the **isOn** flag will be raised, as described above, the  resuming of the **taskWhileOn** activation mechanism is disabled in this class. For that purpose the setTaskWhileOn(const TaskHandle_t) is modified, check the method description for details.
 *
 *@note Due to the short time the **isOn** flag will be raised, as described above, the short time between the **fnWhnTrnOn** function and the **fnWhnTrnOff** function callings must also need to be evaluated.
 *
 * @class SnglSrvcVdblMPBttn
 */
class SnglSrvcVdblMPBttn: public VdblMPBttn{
protected:
   virtual void stOffVddNVUP_Do();	//This provides the calculation for the _validUnvoidPend
   virtual bool updVoidStatus();
public:
   /**
	 * @brief Class constructor
    *
    * @note For the parameters see DbncdDlydMPBttn(GPIO_TypeDef*, const uint16_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
	SnglSrvcVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
	 * @brief Class constructor
    *
    * @note For the parameters see DbncdDlydMPBttn(gpioPinId_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
	SnglSrvcVdblMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
    * @brief Class virtual destructor
    */
   virtual ~SnglSrvcVdblMPBttn();
   /**
    * @brief See DbncdMPBttn::begin(const unsigned long int)
    */
   bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief Sets the task to be run while the object is in the **On state**.
	 *
	 * Sets the task handle of the task to be **resumed** when the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * If the existing value for the task handle was not NULL before the invocation, the method will verify the Task Handle was pointing to a deleted or suspended task, in which case will proceed to **suspend** that task before changing the Task Handle to the new provided value.
	 *
	 * Setting the value to NULL will disable the task execution mechanism.
    *
    *@note The method does not implement any task handle validation for the new task handle, a valid handle to a valid task is assumed as parameter.
    *
    * @note Consider the implications of the task that's going to get suspended every time the MPB goes to the **Off state**, so that the the task to be run might be interrupted at any point of its execution. This implies that the task must be designed with that consideration in mind to avoid dangerous situations generated by a task not completely done when suspended.
    *
    * @warning Take special consideration about the implications of the execution **priority** of the task to be executed while the MPB is in **On state** and its relation to the priority of the calling task, as it might affect the normal execution of the application.
	 */
	virtual void setTaskWhileOn(const TaskHandle_t &newTaskHandle);

};

//==========================================================>>

#endif /* MPBASSWITCH_STM32_H_ */
