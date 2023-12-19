/*
 * @file		: mpbToSwitchSTM32.h
 * @brief	: Header file for
 *
 * @author	: Gabriel D. Goldman
 * @date		: Created on: Nov 6, 2023
 */

#ifndef MPBTOSWITCHSTM32_H_
#define MPBTOSWITCHSTM32_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

//===========================>> Next lines included for developing purposes, corresponding headers must be provided in the instantiating file
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
//===========================>> Previous lines included for developing purposes, corresponding headers must be provided in the instantiating file
//===========================>> Next lines used to avoid CMSIS wrappers
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
//===========================>> Previous lines used to avoid CMSIS wrappers

#define _HwMinDbncTime 20   //Documented minimum wait time for a MPB signal to stabilize
#define _StdPollDelay 10

class DbncdMPBttn {
	static void mpbPollCallback(TimerHandle_t mpbTmrCb);

protected:
	GPIO_TypeDef* _mpbttnPort{};
	uint16_t _mpbttnPin{};
	bool _pulledUp{};
	bool _typeNO{};
	unsigned long int _dbncTimeOrigSett{};

	volatile bool _isPressed{false};
	volatile bool _validPressPend{false};
	volatile bool _isOn{false};
	unsigned long int _dbncTimeTempSett{0};
	unsigned long int _dbncTimerStrt{0};
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

	bool updIsOn();
	bool updIsPressed();
	bool updValidPressPend();

	bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	bool pause();
	bool resume();
	bool end();

	bool setTaskToNotify(TaskHandle_t newHandle);
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

    bool updIsOn();
    bool updIsPressed();
    bool updValidPressPend();

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class LtchMPBttn: public DbncdDlydMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);

protected:
    bool _releasePending{false};
    bool _unlatchPending{false};

public:
    LtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    const bool getUnlatchPend() const;
    bool setUnlatchPend();
    bool updUnlatchPend();

    bool updIsOn();
    bool updIsPressed();
    bool updValidPressPend();

    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
};

//==========================================================>>

class TmLtchMPBttn: public LtchMPBttn{
    static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);

protected:
    bool _tmRstbl {true};
    const unsigned long int _minSrvcTime{100};
    unsigned long int _srvcTime {};
    unsigned long int _srvcTimerStrt{0};

public:
    TmLtchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, const unsigned long int &srvcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    const unsigned long int getSrvcTime() const;
    bool setSrvcTime(const unsigned long int &newSrvcTime);
    bool setTmerRstbl(const bool &isRstbl);

    bool updIsOn();
    bool updIsPressed();
    bool updValidPressPend();
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
    bool updPilotOn();
    bool updWrnngOn();

    bool updIsOn();
    bool updIsPressed();
    bool updValidPressPend();
    bool updUnlatchPend();

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
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin, GPIO_TypeDef* unltchPort, const uint16_t &unltchPin,
        const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0,
        const bool &upulledUp = true, const bool &utypeNO = true, const unsigned long int &udbncTimeOrigSett = 0, const unsigned long int &ustrtDelay = 0);
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    XtrnUnltchMPBttn(GPIO_TypeDef* mpbttnPort, const uint16_t &mpbttnPin,
        const bool &pulledUp = true,  const bool &typeNO = true,  const unsigned long int &dbncTimeOrigSett = 0,  const unsigned long int &strtDelay = 0);
    bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
    bool unlatch();
    bool updIsOn();
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

#endif /* MPBTOSWITCHSTM32_H_ */
