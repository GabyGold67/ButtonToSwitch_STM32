# **Momentary Push Buttons as Switches** Library (mpbAsSwitch)

## Switches make the World go 'round  
### At least... Would you believe me if I told you they Turn On the devices that make it spindle...?  

![Switches](https://github.com/GabyGold67/MomentaryPushButtonsAsSwitches/blob/main/extras/Switches_02b.jpg)

## An STM32-RTOS library that builds switch mechanisms replacements out of simple push buttons.  
By using just a push button (a.k.a. momentary switches or momentary buttons, _**MPB**_ for short from here on) the classes implemented in this library will manage, calculate and update different parameters to **generate the behavior of standard electromechanical switches**. Those parameters include presses, releases, timings, counters or secondary input readings as needed.

The instantiated switch state is updated independently by a standard FreeRTOS software timer (or ESP-RTOS in this case), that keeps the ON/OFF state of the objects created constantly updated. The timer setup is managed by in-class methods, including the possibility to pause, resume or end the timer of each object independently of the others.  
Each class offers a wide range of methods to set, read and modify every significant aspect of each switch mechanism simulated, and the signal received from the push button is debounced for a correct behavior of the event processing.  

The benefits of the use of those simulated switches mechanisms are not just economic, as push buttons come in a wide range of prices and qualities similar to the simulated hardware switches.  

_**In a domestic lighting project**_, for example, detecting after the implementation, through the daily use, that an installed switch was not the best choice when a physical switch is involved, requires for the correction to a best suited kind of switch a bunch of activities to get the change of the switch unit done:
* Searching for the availability of a best suited switch to match our intended functionalities.    
* Getting that different best suited switch, with the time and resources investment involved.  
* Uninstalling the first switch.  
* Maybe modifying the cabling.  
* Installing the new switch...  

While with the simulated switches this situation might be solved by just changing the object class, or modifying the instantiation parameters if needed, and the switch is already changed!!  
* Want to keep the timed **lights-on** longer? Just a parameter.  
* Want to turn off the pilot light because it bothers? Another parameter.  
* Want to be sure the door mechanism isn't kept unlocked with an adhesive tape? Change the switch class.  

_**In an Industrial production machines environment**_ the operator's physical security policies enforcement implies that no time can be wasted and the switch must fit perfectly it's purpose, while still giving the chance to change it's working parameters to adjust the mechanism to changing production operations. In this case the development depending on a simple "OK to proceed"/"The security device is correctly activated" might become a dangerous issue if not correctly isolated and solved as a remove and replace solution. The security switches must comply with issues as:
- Activation enforcement
- Release enforcement
- Time Voidable activation or releases
- Activation sequence enforcement
- Production related disabling of any of the switches while conserving the activation logic... and so many others.  
 
Just to add possibilities, consider that everywhere the **"Momentary Push Button"** is mentioned, any kind of momentary activation signal provider might be used instead: touch sensors, PIR sensors, RFID signals, fingerprint reader and so on...  

## The library implements the following switches mechanisms: ###  
* **Debounced Momentary Button** (a.k.a. Momentary switch, a.k.a. Pushbutton)  
* **Debounced Delayed Momentary Button**  
* **Toggle switch** (a.k.a. alternate, a.k.a. latched)  
* **Timer toggled** (a.k.a. timer switch)  
* **Hinted Timer toggled** (a.k.a. staircase timer switch)
* **External released toggle** (a.k.a. Emergency latched)
* **Time Voidable Momentary Button**  (a.k.a. anti-tampering switch)  
* **Double action On/Off + Slider combo switch**  
* **Double action On/Off + secundary Debounced Delayed MPB**


The system timer will periodically check the input pins associated to the objects and compute the object's output flags, the timer period for that checking is a general parameter that can be changed. The classes provide a callback function to keep the behavior of the objects updated, a valid approach is to create a task for each object to keep status of the outputs associated with each switch updated , or to create a single task to update a number of switches, and act accordingly (turn On/ Off loads, turn On/Off warning signals or other actions). The classes implement a **xTaskNotifyGive()** FreeRTOS macro (a lightweight binary semaphore) to let those tasks implemented to be blocked until they receive the notification of a change, to avoid the tasks polling the objects constantly, freeing resources and processor time. For those not interested in using this mechanism all the attributes of the classes are available without involving it.
Examples are provided for each of the classes, usually as pairs in each example to show possible interactions between different objects, the possible implementations with tasks or with a refreshing mechanism in the main loop() (loopTask), or any other thread created. The object input status checking can be paused, restarted and even ended. If the object's status checking is ended the timer entry will be deleted, to release resources.  
So this library implements the inner mechanisms of some switches, the hardware interface (the load connection, warning outputs, etc.) might be developed differently for every project, or the **_switchHIL_** library (https://github.com/GabyGold67/SwitchesHardwareInterfaceLayer) might be used, as it implements **Hardware Interface Layers** for some usual switches, and for examples of implementations for some classic solutions.  

---  
---  
# **DbncdMPBttn class**
The **Debounced Momentary Button** keeps the ON state since the moment the signal is stable (debouncing process) and until the moment the MPB is released.  

## **Included Methods for DbncdMPBttn class**
|Method | Parameters|
|---|---|
|**_DbncdMPBttn_** |None|
|**_DbncdMPBttn_** |uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**)))|
|**begin()**|(unsigned long int **pollDelayMs**)|
|**clrStatus()**|None|
|**end()**|None|
|**getCurDbncTime()**|None|
|**getIsOn()**|None|
|**getOutputsChange()**|None|
|**getTaskToNotify()**|None|
|**init()** |uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**)))|
|**pause()**|None|
|**resetDbncTime()**|None|
|**resume()**|None|
|**setDbncTime()**|unsigned long int **newDbncTime**|
|**setOutputsChange()**|bool **newOutputChange**|
|**setTaskToNotify()**|TaskHandle_t **newHandle**|

---  
## **Methods definition and use description**
---  

## **DbncdMPBttn**()
### Description:  
Default Class Constructor, creates an instance of the class. This constructor is provided as a tool to create "yet to know parameters" objects, blank objects to use in copies, pointers, etc. The init() method is provided to configure the required attributes to the objects created by this constructor, and there is no other method for setting those basic attributes.  
### Parameters:  
**None**
### Return value:  
The object created.

### Use example:  
**`DbncdMPBttn myDButton();`**  
  
---  
## **DbncdMPBttn**(uint8_t **mpbttnPin**, bool **pulledUp**, bool **typeNO**, unsigned long int **dbncTimeOrigSett**)
### Description:  
Class constructor, creates an instance of the class. There's no need to configure the input pin before calling the method, as the constructor takes care of the task.  
### Parameters:  
**mpbttnPin:** uint8_t (unsigned char), passes the pin number that is connected to the push button. The pin must be free to be used as a digital input.  
**pulledUp:** optional boolean, indicates if the input pin must be configured as INPUT_PULLUP (true, default value), or INPUT_PULLDOWN (false), to calculate correctly the expected voltage level in the input pin. The pin is configured by the constructor so no previous programming is needed. The pin must be free to be used as a digital input, and must be a pin with a **internal pull-up circuit**, as not every GPIO pin has the option.  
**typeNO:** optional boolean, indicates if the mpb is a **Normally Open (NO)** switch (true, default value), or **Normally Closed (NC)** (false), to calculate correctly the expected level of voltage indicating the mpb is pressed.   
**dbncTimeOrigSett:** optional unsigned long integer (uLong), indicates the time (in milliseconds) to wait for a stable input signal before considering the mpb to be pressed (or not pressed). If no value is passed the constructor will assign the minimum value provided in the class, that is 50 milliseconds as it is an empirical value obtained in various published tests.  

### Return value:  
The object created.

### Use example:  
**`DbncdMPBttn myDButton(21, true, true, 100);`**
  
---  
## **begin**(unsigned long int **pollDelayMs**)
### Description:  
Attaches the instantiated object to a timer that monitors the input pin and updates the object status, if the object was not already attached to a timer before. The frequency of that periodic monitoring is passed as a parameter in milliseconds, and is a value that must be small (frequent) enough to keep the object updated, but not so frequent that no other tasks can be executed. A default value is provided based on empirical values obtained in various published tests.  
### Parameters:  
**pollDelayMs:** unsigned long integer (ulong) **optional**, passes the time between polls in milliseconds.  
### Return value:  
true: the object could be attached to a timer, or if it was already attached to a timer when the method was invoked.  
false: the object could not create the needed timer, or the object could not be attached to it.  
### Use example:  
**`myDButton.begin(20);`**  //Attaches de object to a monitoring timer set at 20 milliseconds between checkups.  
  
---  
## **clrStatus**()
### Description:  
Resets some object's attributes to safely resume operations -by using the **resume()** method- after a **pause()** without risking generating false "Valid presses" and "On" situations due to dangling flags or partially ran time counters.  
### Parameters:  
**None**  
### Return value:  
**None**   
### Use example:  
**`myDButton.clrStatus();`**  
  
---  
## **end**()
### Description:  
Detaches the object from the timer that monitors the input pin/s and updates the object status. The timer daemon entry is deleted for the object.  
### Parameters:  
**None**   
### Return value:  
true: the object detachment procedure and timer entry removal was successful.  
false: the object detachment and/or entry removal was rejected by the O.S..  
### Use example:  
**`myDButton.end();`**  
  
---  
## **getCurDbncTime**()  
### Description:  
Returns the current value used for the debouncing process in milliseconds. The original value for the debouncing process used at instantiation time might be changed with the **setDbncTime()** or with the **resetDbncTime()** methods, so this method is provided to get the current value in use.  
### Parameters:  
**None**  
### Return value:  
unsigned long integer: The current debounce time, in milliseconds, being used in the debounce process of the current object.  
### Use example:  
**`myDButton.getCurDbncTime();`**  
  
---  
## **getIsOn**()  
### Description:  
Returns the current value of the _**isOn**_ flag, either true (On) or false (Off). This flag alone might be considered the "Raison d'etre" of all this classes design: the isOn signal is not just the detection of an expected voltage value at a mcu pin, but the combination of that voltage, filtered and verified, for a determined period of time and until a new event modifies that situation.  
### Parameters:  
**None**  
### Return value:  
true: the implemented switch is in **On** state.  
false: the implemented switch is in **Off** state.  
### Use example:  
**`bool lightStatus = myDButton.getIsOn();`**  
  
---  
## **getOutputsChange**()  
### Description:  
Returns the current value of the flag indicating if there have been changes in the output flag/s since last flag resetting. The flag only signals changes have been done, nor which flags nor how many times changes have taken place.  
### Parameters:  
**None**  
### Return value:  
true: there have been output flag changes in the object.  
false: there have been no output flag changes in the object.  
### Use example:  
**`bool outputStatus = myDButton.getOutputsChange();`**  
  
---  
## **getTaskToNotify**()  
### Description:  
Returns the current **TaskHandle** for the task to be notified by the object when its output flags changes (indicating there have been changes in the outputs since last FreeRTOS notification). When the object is created, this value is set to **nullptr** and a valid TaskHandle_t value might be set by using the **setTaskToNotify()** method. The task notifying mechanism will not be used while the task handle keeps the **nullptr** value, in which case the solution implementation will have to include a mechanism to test the object status, ideally through **getOutputsChange()** first, and in the case of a true returned from it a **getIsOn()** and the other methods corresponding to the other flags returned by the object.  
### Parameters:  
**None**  
### Return value:  
TaskHandle_t value pointing to the task implemented to be notified of the change of values of the flags of the objet if it was set to some value not equal to nullptr, or nullptr otherwise.  
### Use example:  
**`TaskHandle_t taskNotified = myDButton.getTaskToNotify();`**  
  
---  
## **init**(uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**))))  
### Description:  
Sets the significant attributes that are needed at instantiation time, and makes the needed operations to get the internal mechanisms active. This method can be applied to objects instantiated with de default constructor only. Having the **mpbttnPin** attribute already set to a valid value blocks the **init()** method of being executed.  
### Parameters:  
**mpbttnPin:** Analog to the **DbncdMPBttn** constructor equally named parameter.  
**pulledUp:** optional boolean, analog to the **DbncdMPBttn** constructor equally named parameter.  
**typeNO:**  analog to the **DbncdMPBttn** constructor equally named parameter.  
**dbncTimeOrigSett:**  analog to the **DbncdMPBttn** constructor equally named parameter.  
### Return value:  
The object with the corresponding attributes set to their initial values.  
### Use example:  
**`myDButton.init(21, true, true, 100);`**  
  
---  
## **pause**()  
### Description:  
Stops the software timer updating the calculation of the object internal flags. The timer will be kept for future use, but the flags will keep their last values and will not be updated until the timer is restarted with the `.resume()` method.  
### Parameters:  
**None**  
### Return value:  
true: the object's timer could be stopped by the O.S..  
false: the object's timer couldn't be stopped by the O.S..  
### Use example:  
**`myDButton.pause();`**  
  
---  
## **resetDbncTime**()  
### Description:  
Resets the debounce time of the object to the value used at instantiation. In case the value was not specified at instantiation the default debounce time value will be used. As the time used at instantiation might be changed with the **setDbncTime()**, this method reverts the value.  
### Parameters:  
**None**  
### Return value:  
true: the value could be reverted.  
false: the value couldn't be reverted due to unexpected situations.  
### Use example:  
**`myDButton.resetDbncTime();`**  
  
---  
## **resume**()  
### Description:  
Restarts the software timer updating the calculation of the object internal flags. The timer will stop its function of computing the flags values after calling the `.pause()` method and will not be updated until the timer is restarted with this method.  
### Parameters:  
**None**  
### Return value:  
true: the object's timer could be restarted by the O.S..  
false: the object's timer couldn't be restarted by the O.S..  
### Use example:  
**`myDButton.resume();`**  
  
---  
### **setDbncTime**(unsigned long int **newDbncTime**)
### Description:  
Sets a new time for the debouncing period. The value must be equal or greater than the minimum empirical value set as a property for all the classes, 20 milliseconds. A long debounce time will produce a delay in the press event generation, making it less "responsive".  
### Parameters:  
**newDbncTime:** unsigned long integer, the new debounce value for the object.  
### Return value:  
true: the new value is in the accepted range and the change was made.  
false: the value was already in use, or was out of the accepted range, no change was made.  
### Use example:  
**`myDButton.setDbncTime(200);`** //Sets the new debounce time and returns true.  
**`myDButton.setDbncTime(15);`** //Returns false and the debounce time is kept unchanged.  
  
---  
### **setOutputsChange**(bool **newOutputChange**)
### Description:  
Sets the value of the flag indicating if a change took place in any of the output flags (IsOn included). The usual path for the _outputsChange flag is to be set to true by any method changing an output flag, the callback function signaled to take care of the hardware actions because of this changes clears back _outputsChange after taking care of them. In the unusual case the developer wants to "intercept" this sequence, this method is provided to set (true) or clear (false) _outputsChange value.  
### Parameters:  
**newOutputChange:** boolean, the new value to set the _outputsChange flag to.  
### Return value:  
true: The value change was successful.  
false: The value held by _outputsChange and the newOutputChange parameter are the same, no change was then produced.  
### Use example:  
**`myDButton.setOutputsChange(false);`** //Returns true if the newOutputChange if different to the _outputsChange value, and the change is made.  
  
---  
### **setTaskToNotify**(TaskHandle_t **newHandle**)
### Description:  
Sets the pointer to the task to be notified by the object when its output flags changes (indicating there have been changes in the outputs since last FreeRTOS notification). When the object is created, this value is set to **nullptr** and a valid TaskHandle_t value might be set by using this method. The task notifying mechanism will not be used while the task handle keeps the **nullptr** value. After the TaskHandle value is set it might be changed to point to other task 
### Parameters:  
**newHandle:** TaskHandle_t, a valid handle of an actual existent task/thread running. There's no provided exception mechanism for dangling pointer errors caused by a pointed task being deleted and/or stopped.  
### Return value:  
true: A TaskHandle_t type was passed to the object to be it's new pointer to the task to be messaged when a change in the output flags occur. There's no checking for the validity of the pointer, if it refers to an active task/thread whatsoever.  
false: The value passed to the method was **nullptr**, and that's the value will be stored, so the whole RTOS messaging mechanism won't be used.  
### Use example:  
**`TaskHandle_t myHwUpdtTask;`**  
**`[...]`**  
**`myDButton.setTasToNotify(myHwUpdtTask);`**  
  
---  
---  
# **DbncdDlydMPBttn class**  
The **Debounced Delayed Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released. The reasons to add the delay are design related and are usually used to avoid unintentional presses, or to give some equipment (load) that needs time between repeated activations the benefit of the pause. If the push button is released before the delay configured, no press is registered at all. The delay time in this class as in the other that implement it, might be zero (0), defined by the developer and/or modified in runtime.  

# **Added Methods for DbncdDlydMPBttn class**  
  
|Method | Parameters|
|---|---|
|**_DbncdDlydMPBttn_** |None|
|**_DbncdDlydMPBttn_** |uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**))))|
|**getStrtDelay()**|None|
|**init** |uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**))))|
|**setStrtDelay()**|(unsigned long int **newStrtDelay**)|
  
---  
### **DbncdDlydMPBttn**(uint8_t **mpbttnPin**, bool **pulledUp**, bool **typeNO**, unsigned long int **dbncTimeOrigSett**, unsigned long int **strtDelay**)
### Description:  
Class constructor, creates an instance of the class for each **Debounced Delayed Momentary Push Button**. There's no need to configure the input pin before calling the method, as the constructor takes care of the task.  
### Parameters:  
**mpbttnPin:** uint8_t (unsigned char), passes the pin number that is connected to the push button. The pin must be free to be used as a digital input.  
**pulledUp:** optional boolean, indicates if the input pin must be configured as INPUT_PULLUP (true, default value), or INPUT_PULLDOWN(false), to calculate correctly the expected voltage level in the input pin. The pin is configured by the constructor so no previous programming is needed. The pin must be free to be used as a digital input.  
**typeNO:** optional boolean, indicates if the mpb is a **Normally Open** switch (true, default value), or **Normally Closed** (false), to calculate correctly the expected level of voltage indicating the mpb is pressed.   
**dbncTimeOrigSett:** optional unsigned long integer (uLong), indicates the time (in milliseconds) to wait for a stable input signal before considering the mpb to be pressed. If no value is passed the constructor will assign the minimum value provided in the class, that is 50 milliseconds as it is a empirical value obtained in various published tests.  
**strtDelay:** optional unsigned long integer (uLong), indicates the time (in milliseconds) to wait after the debouncing is completed and before rising the ON flag. If no value is passed the constructor will assign 0 milliseconds, and the resulting behavior will be the same as implementing a DbncdMPBttn. If the mpb is released before completing the strtDelay time, no ON signal will be produced.  
### Return value:  
The object created.  
### Use example:  
**`DbncdMPBttn myDDButton(21, true, true, 100, 500);`**  
  
---  
## **getStrtDelay**()  
### Description:  
Returns the current value of time used by the object to rise the isOn flag, after the debouncing process ends, in milliseconds. If the MPB is released before completing the debounce **and** the delay time, no press will be detected by the object, and the isOn flag will not be rised. The original value for the delay process used at instantiation time might be changed with the **setStrtDelay()** method, so this method is provided to get the current value in use.  
### Parameters:  
**None**  
### Return value:  
unsigned long integer: The current delay time, in milliseconds, being used before rising the isOn flag, after the debounce process of the current object.  
### Use example:  
**`unsigned long curDelay {myDButton.getStrtDelay()};`**  //Stores the current Start delay value in the curDelay variable  
  
---  
### **setStrtDelay**(unsigned long int **newStrtDelay**)  
### Description:  
Sets a new time for the Start Delay period. The value must be equal or greater than 0 milliseconds, in the case of equal to 0 the instantiated object will act as a DbncdMPBttn class object. A long Start Delay time will produce a long delay in the press event generation, making it less "responsive".  
### Parameters:  
**newStrtDelay:** unsigned long integer, the new start delay value for the object.  
### Return value:  
true: the new value is different than the previously set value, so the change was made.  
false: the value passed is equal to the one already in use, no change was made.  
### Use example:  
**`myDButton.setStrtDelay(0);`** //Sets the new debounce time to 0, there will no time added to the debounce process, the MPB will act as a DbncdMPBttn.  
**`myDButton.setStrtDelay(myDButton.getStrtDelay() + 15);`** //Changes the current start delay time by adding 15 milliseconds, and returns true.  
  
---  
---  
# **LtchMPBttn class**  
The **Toggle switch**  keeps the ON state since the moment the signal is stable (debouncing + Delay process), and keeps the ON state after the push button is released and until it is pressed once again. So this simulates a simple On-Off switch like the ones used to turn on/off a room light. One of the best things about the software simulated switch is that any amount of switches might be set up in a parallel configuration, so that an unlimited number of entrances or easy accessible points can each have a switch to turn on/off the same resource.  
## **Added Methods for LtchMPBttn class**  
|Method | Parameters|
|---|---|
|**_LtchMPBttn_** |uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**))))|
|**getUnlatchPend()**|None|
|**setUnlatchPend()**|None|
  
---  
## **getUnlatchPend**()  
### Description:  
Returns the current value of unlatch pending flag, whose boolean value indicates if the conditions were given to unlatch a LtchMPBttn object. The periodical timer event that takes care of keeping the internal and external objects'flags updated, takes care of the actions required to set the flag and eventually acting on this flag value. This method makes it possible to know the flag value to give the developer a tool to generate other events his development might required, but not by modifying the class.  
### Parameters:  
**None**  
### Return value:  
true: The current object's unlatchPending flag is set.  
false: The current object's unlatchPending flag is reset.  
### Use example:  
**`if(myDButton.getUnlatchPend()){...}`**  //Executes some code if the unlatchPending flag is set.  
  
---  
### **setUnlatchPend**()
### Description:  
Sets to **true** the value of the internal flag **unlatchPending**. It is the equivalent to physically executing the action that releases the latch that keeps the isOn signal lactched, in this case it will be the equivalent to pressing the MPB while the isOn signal is true. This method gives an alternative to generate the appropiate signal to unlatch the LtchMPBttn.  
### Parameters:  
**None**  
### Return value:  
true: the unlatchPending value was set.  
false: Abnormal situation, the unlatchPending flag could not be set.  
### Use example:  
**`myDButton.setUnlatchPend();`**  

---  
---  

# **TmLtchMPBttn class**  
The **Timer toggled** or **Timer Switch**, keeps the ON state since the moment the signal is debounced, and keeps the state during a set time, the switch time is set at instantiation, and can be modified through the provided methods. The switch implementation gives the option to allow to reset the timer before reaches the time limit if the push button is pressed again.  
  
## **Added Methods for TmLtchMPBttn class**  
|Method | Parameters|
|---|---|
|**_TmLtchMPBttn_** |uint8_t **mpbttnPin**, unsigned long int **actTime**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**))))|
|**getSvcTime()**|None|
|**setSvcTime()**|unsigned long int **newSvcTime**|
|**setTmerRstbl()**|bool **isRstbl**|

---  
---  

# **HntdTmLtchMPBttn class**
The **Hinted Timer toggled**, or **Staircase Timer Switch**, keeps the ON state since the moment the signal is debounced, and keeps the state during a set time, the switch time is set at instantiation, and can be modified through the provided methods. The switch implementation gives the option to allow to reset the timer before it gets to the end if the push button is pressed, the option to give a warning when the time is close to the end through a second flag (remaining time is defined as a percentage of the total ON time and it's configurable), and the possibility to set a third signal ON while the switch is off, just like the pilot light (hint) in a staircase timer switch. The warning signal is independent of the off hint.  

## **Added Methods for HntdTmLtchMPBttn class**  

|Method | Parameters|
|---|---|
|**_HntdTmLtchMPBttn_** |uint8_t **mpbttnPin**, unsigned long int **actTime**(, unsigned int **wrnngPrctg**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**)))))|  
|**getPilotOn()**|None|
|**getWrnngOn()**|None|
|**setKeepPilot()**|bool **keepPilot**|
|**updPilotOn()**|None|
|**updWrnngOn()**|None|
  
---  
---  

# **Added Methods for XtrnUnLtchMPBttn class**
The **External released toggle** (a.k.a. Emergency latched), keeps the On state since the moment the signal is debounced, and until an external signal is received. This kind of switch is used when an "abnormal situation" demands the push of the switch On, but a higher authority is needed to reset it to Off from a different signal source. Smoke, flood, intrusion alarms and "last man locks" are some examples of the use of this switch. As the external release signal can be physical or logical generated it can be implemented to be received from a switch or a remote signal of any usual kind. It's implemented in the  **XtrnUnltchMPBttn** class.  
  
|Method | Parameters|
|---|---|
|**_XtrnUnLtchMPBttn_** |uint8_t **mpbttnPin**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**))))|
|**_XtrnUnLtchMPBttn_** |uint8_t **mpbttnPin**, DbncDlydMPBttn* **unltchBttn**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**))))|
|**unlatch()**|None|

---  
---  
# **Added Methods for TmVdblMPBttn class**  
The **Time Voidable Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released, or until a preset time in the ON state is reached. Then the switch will return to the Off position until the push button is released and pushed back. This kind of switches are used to activate limited resources related management or physical safety devices, and the possibility of a physical blocking of the switch to extend the ON signal artificially beyond designer's plans is highly undesired. Water valves, door unlocking mechanisms, hands-off security mechanisms, high power heating devices are some of the usual uses for these type of switches. It's implemented in the **TmVdblMPBttn** class.  

|Method | Parameters|
|---|---|
|**_TmVdblMPBttn_** |uint8_t **mpbttnPin**, unsigned long int **voidTime**(, bool **pulledUp**(, bool **typeNO**(, unsigned long int **dbncTimeOrigSett**(, unsigned long int **strtDelay**(, bool **isOnDisabled**)))))|
|**disable()**|None|
|**enable()**|None|
|**getIsEnabled()**|None|
|**getIsOnDisabled()**|None|
|**getIsVoided()**|None|
|**getVoidTime()**|None|
|**setIsEnabled()**|bool **enabledValue**|
|**setIsOnDisabled()**|bool **isOnDisabledValue**|
|**setIsVoided()**|bool **voidValue**|
|**setVoidTime()**|None|
|**updIsVoided()**|None|
