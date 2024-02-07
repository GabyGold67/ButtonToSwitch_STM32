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
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
//===========================>> Previous lines included for developing purposes, corresponding headers must be provided for the production platform/s

//===========================>> Next lines used to avoid CMSIS wrappers
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
//===========================>> Previous lines used to avoid CMSIS wrappers

#define _HwMinDbncTime 20  // Documented minimum wait time for a MPB signal to stabilize to consider it pressed or released (in milliseconds)
#define _StdPollDelay 10	// Reasonable time between polls for MPBs switches (in milliseconds)
#define _MinSrvcTime 100	// Minimum valid time value for service/active time for Time Latched MPBs to avoid stability issues relating to debouncing, releasing and other timed events
#define _InvalidPinNum 0xFFFF	// Value to give as "yet to be defined", the "Valid pin number" range and characteristics are development platform and environment dependable

struct gpioPinId{	// Type used to keep development as platform independent as possible,
	GPIO_TypeDef* portId;
	uint16_t pinNum;
};

//struct gpioPinId{	// Type used to keep development as platform independent as possible,
//	uint8_t pinNum;
//};


class DbncdMPBttn {
	enum fdaDmpbStts {stOffNotVPP, stOffVPP,  stOn, stOnVRP};
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);
protected:
	GPIO_TypeDef* _mpbttnPort{};
	uint16_t _mpbttnPin{};
	bool _pulledUp{};
	bool _typeNO{};
	unsigned long int _dbncTimeOrigSett{};

	volatile bool _isOn{false};
	volatile bool _isPressed{false};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};
	unsigned long int _dbncRlsTimerStrt{0};
	unsigned long int _dbncRlsTimeTempSett{0};
	unsigned long int _dbncTimeTempSett{0};
	unsigned long int _dbncTimerStrt{0};
	fdaDmpbStts _dmpbFdaState {stOffNotVPP};
	const unsigned long int _stdMinDbncTime {_HwMinDbncTime};
	TimerHandle_t _mpbPollTmrHndl {NULL};	// Std-FreeRTOS
	char _mpbPollTmrName [18] {'\0'};
	volatile bool _outputsChange {false};
	TaskHandle_t _taskToNotifyHndl {NULL};	// Std-FreeRTOS
	const bool getIsPressed() const;
public:
	DbncdMPBttn();
	DbncdMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	virtual ~DbncdMPBttn();
	void clrStatus();
	const unsigned long int getCurDbncTime() const;
	const bool getIsOn () const;
	const bool getOutputsChange() const;
	const TaskHandle_t getTaskToNotify() const;
	bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	bool resetDbncTime();
	bool setDbncTime(const unsigned long int &newDbncTime);
	bool setOutputsChange(bool newOutputChange);
	bool setTaskToNotify(TaskHandle_t newHandle);

	bool updIsPressed();
	void updFdaState();
	bool updValidPressesStatus(unsigned long int totXtraDelays = 0);

	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	bool pause();
	bool resume();
	bool end();
};

//==========================================================>>

class DbncdDlydMPBttn: public DbncdMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
protected:
    unsigned long int _strtDelay {0};
public:
    DbncdDlydMPBttn();
    DbncdDlydMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    unsigned long int getStrtDelay();
    bool init(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    bool setStrtDelay(const unsigned long int &newStrtDelay);

    bool updValidPressesStatus(unsigned long int totXtraDelays = 0);

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class LtchMPBttn: public DbncdDlydMPBttn{
	enum fdaLmpbStts {stOffNotVPP, stOffVPP,  stOnNVRP, stOnVRP, stOnNVPP, stOnVPP, stOffNVRP, stOffVRP};
	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
protected:
    bool _validUnlatchPend{false};
    bool _isLatched{false};	//Probably unneeded
    bool unlatch();
public:
	LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	const bool getUnlatchPend() const;
	bool setUnlatchPend();
	void updFdaState();
	bool updValidPressesStatus(unsigned long int totXtraDelays = 0);

//	bool updIsOn();
//	bool updValidPressPend();

	void updFdaState();
//	bool updValidPressesStatus(unsigned long int totXtraDelays = 0);

	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class TmLtchMPBttn: public LtchMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
protected:
    bool _tmRstbl {true};
    unsigned long int _srvcTime {};
    unsigned long int _srvcTimerStrt{0};
public:
    TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    const unsigned long int getSrvcTime() const;
    bool setSrvcTime(const unsigned long int &newSrvcTime);
    bool setTmerRstbl(const bool &newIsRstbl);

    bool updIsOn();
    bool updUnlatchPend();

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class HntdTmLtchMPBttn: public TmLtchMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
protected:
    bool _wrnngOn {false};
    bool _keepPilot{false};
    bool _pilotOn{false};
    unsigned int _wrnngPrctg {0};
    unsigned long int _wrnngMs{0};
public:
    HntdTmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &actTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    const bool getPilotOn() const;
    const bool getWrnngOn() const;
    bool setSrvcTime(const unsigned long int &newActTime);
    bool setKeepPilot(const bool &newKeepPilot);
    bool setWrnngPrctg (const unsigned int &newWrnngPrctg);
    bool updPilotOn();
    bool updWrnngOn();

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class XtrnUnltchMPBttn: public LtchMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
protected:
    bool _unltchPulledUp{};
    bool _unltchTypeNO{};
    DbncdDlydMPBttn* _unLtchBttn {nullptr};
public:
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
    bool updIsOn();
    bool unlatch();
    bool updUnlatchPend();
};

//==========================================================>>

class VdblMPBttn: public DbncdDlydMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCb);
protected:
    bool _isEnabled{true};
    bool _isOnDisabled{false};
    bool _isVoided{false};
public:
    VdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    virtual ~VdblMPBttn();
    void clrStatus();
    bool enable();
    bool disable();
    const bool getIsEnabled() const;
    const bool getIsOnDisabled() const;
    const bool getIsVoided() const;
    bool setIsEnabled(const bool &newEnabledValue);
    bool setIsOnDisabled(const bool &newIsOnDisabled);
    bool setIsVoided(const bool &newVoidValue);
    virtual bool updIsVoided() = 0;
};

//==========================================================>>

class TmVdblMPBttn: public VdblMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCb);
protected:
    unsigned long int _voidTime;
    unsigned long int _voidTmrStrt{0};
public:
    TmVdblMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const bool &isOnDisabled = false);
    virtual ~TmVdblMPBttn();
    void clrStatus();
    const unsigned long int getVoidTime() const;
    bool setIsEnabled(const bool &newEnabledValue);
    bool setIsVoided(const bool &newVoidValue);
    bool setVoidTime(const unsigned long int &newVoidTime);
    bool updIsOn();
    bool updIsPressed();
    virtual bool updIsVoided();
    bool updValidPressPend();

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class SldrLtchMPBttn: public LtchMPBttn{
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);
	enum fdaSldrStts {stOffNotVPP, stOffVPP,  stOnMPBRlsd, stOnSldrMod, stOnTurnOff};
protected:
	bool _autoChngDir{true};
	uint16_t _curOtptVal{};
	bool _curSldrDirUp{true};
	bool _curValMax{false};
	bool _curValMin{false};
	unsigned long _otptSldrSpd{1};
	uint16_t _otptSldrStpSize{0x01};
	uint16_t _otptValMax{0xFFFF};
	uint16_t _otptValMin{0x00};
	unsigned long _sldrActvDly {1500};
	fdaSldrStts _sliderFdaState {stOffNotVPP};
	unsigned long _sldrTmrStrt {0};
	bool _validSlidePend{false};
public:
   SldrLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0, const uint16_t initVal = 0xFFFF);
   ~SldrLtchMPBttn();
	uint16_t getCurOtptVal();
   uint16_t getOtptValMax();
	uint16_t getOtptValMin();
	uint16_t getOtptSldrStpSize();
	unsigned long getSldrActvDly();
	unsigned long getOtptSldrSpd();
	bool getSldrDirUp();
	bool setOtptValMax(const uint16_t &newVal);
	bool setOtptValMin(const uint16_t &newVal);
	bool setOtptSldrStpSize(const uint16_t &newVal);
	bool setOtptSldrSpd(const uint16_t &newVal);
	bool setSldrActvDly(const unsigned long &newVal);
	bool setSldrDirUp(const bool &newVal);
	void updFdaState();
	bool updValidPressPend();

   bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

#endif /* MPBASSWITCH_STM32_H_ */
