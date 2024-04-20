/**
 * @file		: mpbAsSwitch_STM32.h
 * @brief	: Header file for the MpbAsSwitch_STM32 library classes
 *
 * @author	: Gabriel D. Goldman
 * @date		: Created on: Nov 6, 2023
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
 * 			A GPIO pin identification is MCU and framework dependent, so the quantity and type of parameters
 * 			vary. This structure provides an interface to abstract them as a single identification parameter
 *
 * @struct gpioPinId_t
 *
 */
struct gpioPinId_t{	// Type used to keep GPIO pin identification as a single parameter, as platform independent as possible
	GPIO_TypeDef* portId;	/**< The port identification as a pointer to a GPIO_TypeDef information structure*/
	uint16_t pinNum;	/**< The number of pin represented as a one bit set binary with the set bit position indicating the pin number*/
};

//===========================>> BEGIN General use function prototypes

uint8_t singleBitPosNum(uint16_t mask);

//===========================>> END General use function prototypes

constexpr int genNxtEnumVal(const int &curVal, const int &increment){return (curVal + increment);}

//==========================================================>> BEGIN Classes declarations

/**
 * @brief 	Implements a debounced deglitched Pushbutton from the original raw signal received as input
 *
 * 			This class provides the resources needed to process a momentary digital input signal -as the one
 * 			provided by a MPB (momentary push button)- returning a clean signal to be used as a switch,
 * 			implementing the needed services to replace a wide range of physical related switch characteristics.
 *
* 	@class	DbncdMPBttn
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
   bool _isEnabled{true};
	volatile bool _isOn{false};
   bool _isOnDisabled{false};
	volatile bool _isPressed{false};
	fdaDmpbStts _mpbFdaState {stOffNotVPP};
	TimerHandle_t _mpbPollTmrHndl {NULL};
//	char _mpbPollTmrName [18] {'\0'};
	std::string _mpbPollTmrName {""};
	volatile bool _outputsChange {false};
	bool _prssRlsCcl{false};
	bool _sttChng {true};
	TaskHandle_t _taskToNotifyHndl {NULL};
	bool _validDisablePend{false};
	bool _validEnablePend{false};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};

	void clrSttChng();
	const bool getIsPressed() const;
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);
   bool setIsEnabled(const bool &newEnabledValue);
	void setSttChng();
	virtual void updFdaState();
	bool updIsPressed();
	virtual bool updValidPressesStatus();
public:
	DbncdMPBttn();
	DbncdMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	DbncdMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	virtual ~DbncdMPBttn();
	void clrStatus(bool clrIsOn = true);
   /**
	 * @brief	Disables the input signal processing, ignoring the changes of its values.
    *
    * @return	True if the object was enabled, the method invocation disabled it.
    */
	bool disable();
   /**
	 * @brief	Enables the input signal processing.
    *
    * @return	True if the object was disabled, the method invocation enabled it.
    */
   bool enable();
	/**
	 * @brief 	Gets the current debounce time
	 *
	 * @return	The current debounce time in milliseconds
	 */
   const unsigned long int getCurDbncTime() const;
   const bool getIsEnabled() const;
	const bool getIsOn () const;
   const bool getIsOnDisabled() const;
	const bool getOutputsChange() const;
	const TaskHandle_t getTaskToNotify() const;
	bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	bool init(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	bool resetDbncTime();
	bool resetFda();
	/**
	 * @brief	Sets a new debounce time
	 *
	 *	Sets a new time for the debouncing period. The value must be equal or greater than the minimum empirical value set as a property for all the classes, 20 milliseconds. A long debounce time will produce a delay in the press event generation, making it less "responsive".
	 *
	 * @param newDbncTime	unsigned long integer, the new debounce value for the object.
	 * @return	A boolean indicating if the debounce time change was successful
	 *
	 * true: the new value is in the accepted range and the change was made.
	 * false: the value was already in use, or was out of the accepted range, no change was made.
	 */
	bool setDbncTime(const unsigned long int &newDbncTime);
   bool setIsOnDisabled(const bool &newIsOnDisabled);
   bool setOutputsChange(bool newOutputChange);
	bool setTaskToNotify(TaskHandle_t newHandle);

	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	bool pause();
	bool resume();
	bool end();
};

//==========================================================>>

class DbncdDlydMPBttn: public DbncdMPBttn{
protected:
    unsigned long int _strtDelay {0};

    virtual bool updValidPressesStatus();
public:
    DbncdDlydMPBttn();
    DbncdDlydMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    DbncdDlydMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
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
	unsigned long getScndModActvDly();
	bool setScndModActvDly(const unsigned long &newVal);

   bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class DDlydLtchMPBttn: public DblActnLtchMPBttn{
protected:
   bool _isOn2{false};

   virtual void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do();
   virtual void stOnEndScndMod_Out();
public:
   DDlydLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   DDlydLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   ~DDlydLtchMPBttn();
   bool getIsOn2();
};

//==========================================================>>

class SldrLtchMPBttn: public DblActnLtchMPBttn{

protected:
	bool _autoSwpDirOnEnd{true};	// Changes slider direction when reaches _otptValMax or _otptValMin
	bool _autoSwpDirOnPrss{false};// Changes slider direction each time it enters slider mode
	bool _curSldrDirUp{true};
	uint16_t _otptCurVal{};
	unsigned long _otptSldrSpd{1};
	uint16_t _otptSldrStpSize{0x01};
	uint16_t _otptValMax{0xFFFF};
	uint16_t _otptValMin{0x00};
	unsigned long _sldrTmrStrt {0};

   void stOnStrtScndMod_In();
   virtual void stOnScndMod_Do();
	bool _setSldrDir(const bool &newVal);
public:
   SldrLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
   SldrLtchMPBttn(gpioPinId_t mpbttnPinStrct, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
   ~SldrLtchMPBttn();
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
