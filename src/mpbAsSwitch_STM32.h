/*
 * @file		: mpbAsSwitch_STM32.h
 * @brief	: Header file for
 *
 * @author	: Gabriel D. Goldman
 * @date		: Created on: Nov 6, 2023
 */

#ifndef MPBASSWITCH_STM32_H_
#define MPBASSWITCH_STM32_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

//===========================>> Next lines included for developing purposes, corresponding headers must be provided for the production platform/s
#ifndef __STM32F4xx_HAL_H
#include "stm32f4xx_hal.h"
#endif

#ifndef __STM32F4xx_HAL_GPIO_H
#include "stm32f4xx_hal_gpio.h"
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

struct gpioPinId_t{	// Type used to keep GPIO pin identification as a single parameter, as platform independent as possible
	GPIO_TypeDef* portId;
	uint16_t pinNum;
};

//===========================>> BEGIN General use function prototypes

uint8_t singleBitPosition(uint16_t mask);

//===========================>> END General use function prototypes

constexpr int genNxtEnumVal(const int &curVal, const int &increment){return (curVal + increment);}

class DbncdMPBttn {
protected:
	enum fdaDmpbStts {stOffNotVPP,
							stOffVPP,
							stOn,
							stOnVRP
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
	char _mpbPollTmrName [18] {'\0'};
	volatile bool _outputsChange {false};
	bool _prssRlsCcl{false};
	bool _sttChng {true};
	TaskHandle_t _taskToNotifyHndl {NULL};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};

	void clrSttChng();
	const bool getIsPressed() const;
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);
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
   bool disable();
   bool enable();
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
	bool setDbncTime(const unsigned long int &newDbncTime);
   bool setIsEnabled(const bool &newEnabledValue);
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
		stOffVURP
	};
	bool _isLatched{false};
	fdaLmpbStts _mpbFdaState {stOffNotVPP};
	bool _trnOffASAP{true};
	bool _validUnlatchPend{false};
	bool _validUnlatchRlsPend{false};

	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
	virtual void updFdaState();
	virtual void updValidUnlatchStatus() = 0;

	virtual void stOffNotVPP_Out(){};
	virtual void stOffVPP_Out(){};
	virtual void stOffVURP_Out(){};
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
    virtual void stOffVPP_Out();
    virtual void stOffNotVPP_Out();
    virtual void stOffVURP_Out();
public:
    TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
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

	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
	bool updPilotOn();
	bool updWrnngOn();
public:
	HntdTmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &actTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
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
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
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
		stOnStrtScndMod = genNxtEnumVal(stOnMPBRlsd,100),
		stOnScndMod = genNxtEnumVal(stOnStrtScndMod,100),
		stOnEndScndMod = genNxtEnumVal(stOnScndMod,100),
		stOnTurnOff = genNxtEnumVal(stOnEndScndMod,100)
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

//Gaby must start here!!

class VdblMPBttn: public DbncdDlydMPBttn{
protected:
	enum fdaVmpbStts{
 		stOffNotVPP,
 		stOffVPP,
 		stOnNVRP,
		//--------
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
    bool _validUnvoidPend{false};

    static void mpbPollCallback(TimerHandle_t mpbTmrCb);
    bool setIsEnabled(const bool &newEnabledValue);
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

//    bool setVoided(const bool &newVoidValue);
    virtual void stOffNotVPP_In();
    virtual void stOffVddNVUP_Do();	//This provides a setting point for calculating the _validUnvoidPend
    virtual void stOffVPP_Do();	// This provides a setting point for the voiding mechanism to be started
    bool updIsPressed();
    virtual bool updVoidStatus();
public:
    TmVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    virtual ~TmVdblMPBttn();
    void clrStatus();
    const unsigned long int getVoidTime() const;
    bool setIsEnabled(const bool &newEnabledValue);
//    bool setIsNotVoided();
//    bool setIsVoided();
    bool setVoidTime(const unsigned long int &newVoidTime);

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

#endif /* MPBASSWITCH_STM32_H_ */
