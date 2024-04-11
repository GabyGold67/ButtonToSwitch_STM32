# **Momentary Push Buttons as Switches** Library (mpbAsSwitch)

## An STM32-RTOS library that builds switch mechanisms replacements out of simple push buttons.  
By using just a push button (a.k.a. momentary switches or momentary buttons, _**MPB**_ for short from here on) the classes implemented in this library will manage, calculate and update different parameters to **generate the behavior of standard electromechanical switches**. Those parameters include presses, releases, timings, counters or secondary input readings as needed.

![Classes](https://github.com/GabyGold67/mpbAsSwitch_STM32/blob/master/Extras/MomentaryPushButtonUMLClassesOnly.jpg)

## The library implements the following switches mechanisms: ###  
* **Debounced Momentary Button** (a.k.a. Momentary switch, a.k.a. Pushbutton)  
* **Debounced Delayed Momentary Button**  
* **Toggle switch** (a.k.a. alternate, a.k.a. latched)  
* **Timer toggled** (a.k.a. timer switch)  
* **Hinted Timer toggled** (a.k.a. staircase timer switch)
* **External released toggle** (a.k.a. Emergency latched)
* **Time Voidable Momentary Button**  (a.k.a. anti-tampering switch)  
* **Double action On/Off + Slider combo switch**  (a.k.a. off/on/dimmer, a.k.a. off/on/volume radio switch)
* **Double action On/Off + secondary Debounced Delayed MPB**

Each instantiated object returns a debounced, deglitched, clean "isOn" signal based on the expected behavior of the simulated switch mechanism. 

The system timer will periodically check the input pins associated to the objects and compute the object's output flags, the timer period for that checking is a general parameter that can be changed. The classes provide a callback function to keep the behavior of the objects updated, a valid approach is to create a task for each object to keep status of the outputs associated with each switch updated , or to create a single task to update a number of switches, and act accordingly (turn On/ Off loads, turn On/Off warning signals or other actions). The classes implement a **xTaskNotifyGive()** FreeRTOS macro (a lightweight binary semaphore) to let those tasks implemented to be blocked until they receive the notification of a change, to avoid the tasks polling the objects constantly, freeing resources and processor time. For those not interested in using this mechanism all the attributes of the classes are available without involving it.
Examples are provided for each of the classes

