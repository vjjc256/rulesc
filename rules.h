#ifndef _RULES_H
#define _RULES_H

#include <inttypes.h>
#include <stdbool.h>
#include "relay.h" 

//256*16 = 4K (a sector size)
#define MAX_NUMBER_OF_RULES (8)
#define MAX_NUMBER_OF_PERIODS (14)

#define Second        (1)
#define Minute        (60*Second)
#define Hour          (60*Minute)
#define Day           (24*Hour)
#define SecondsInWeek (Hour*24*7)

typedef uint8_t DayOfWeekT;

typedef enum {
	Sunday = 1,
	Monday = Sunday << 1,
	Tuesday = Monday << 1,
	Wednesday = Tuesday << 1,
	Thursday = Wednesday << 1,
	Friday = Thursday << 1,
	Saturday = Friday << 1,
} DaysT;

#define Weekends     (Saturday | Sunday)
#define Weekdays     (Monday | Tuesday | Wednesday | Thursday | Friday)
#define SchoolNights (Sunday | Monday | Tuesday | Wednesday | Thursday)
#define Everyday     (Sunday | Monday | Tuesday | Wednesday | Thursday | Friday | Saturday)

typedef enum {
	StateOff = 0,
	StateOn = 1,
} StateT;

typedef enum {
	OverrideNone = 0,
	OverrideOff = 1,
	OverrideOn = 2,
} OverrideT;

typedef struct {
	uint8_t flagCount;
	uint8_t periods[5 * MAX_NUMBER_OF_PERIODS];
} TimelineT;

typedef enum {
	PreferenceNone      = 0,
	PreferenceStrongOff = 1,
	PreferenceStrongOn  = 2,
	PreferenceWeakOff   = 3,
	PreferenceWeakOn    = 4,
} PreferenceT;

typedef struct {
	PreferenceT Preference;
	PreferenceT DeletedPreference;
	int8_t PeriodIndex;
	bool WeekBarrier;
	bool DisableOverride;
	bool ClearOverride;
	bool LogUsage;
} RuleResultT;

typedef enum {
	RuleEventTypeUnknown   = 0,
	RuleEventTypeActivated  = 1,
	RuleEventTypeDeactivated     = 2,
	RuleEventTypeDeleted = 3,
	RuleEventTypeManualOverride = 4,
	RuleEventTypeRuleOverride = 5,
} RuleEventTypeT;

typedef enum {
	RuleTypeUnknown   = 0,
	RuleTypeSchedule  = 1,
	RuleTypeProximity = 2,
	RuleTypeUsage     = 3,
} RuleTypeT;

typedef enum {
	proximityDirectionHome = 0,
	proximityDirectionAway = 1,
} ProximityDirectionT;

typedef struct {
	uint32_t now;
	
	// TODO: these should be preserved in flash across reboots
	StateT state;
	OverrideT override;
	uint64_t RuleID;
	RuleResultT RuleResult;
	
	bool home;
	
	uint32_t LastRulePrefChange;
	uint32_t totalUsed;
	
	RuleResultT RuleResults[MAX_NUMBER_OF_RULES];
	uint64_t    RuleIDIndex[MAX_NUMBER_OF_RULES];

	RelayCoilNumberT relay;  //the relay for this context, either COIL_1 or COIL_2
} ContextT;

typedef struct RuleInterfaceT {
	uint64_t (*Id)(struct RuleInterfaceT *rule);
	RuleTypeT (*Type)(struct RuleInterfaceT *rule);

	RuleResultT (*Run)(ContextT *ctx, struct RuleInterfaceT *rule, uint32_t time, RuleResultT previous);

	//the fields below are strictly for the C implementation
	void *ruleData;  //pointer to one of the rule types, i.e. ProximityRuleT, UsageRuleT, or ScheduleRuleT.
	RelayCoilNumberT target;  //the target for this rule, either COIL_1, COIL_2, or COIL_BOTH
} RuleInterfaceT;

typedef struct {
	uint64_t id;
	RuleTypeT ruleType;
	TimelineT timeline;
	
	RuleInterfaceT interface;  //provides equivalent to the 'Rule' interface in Golang
} RuleT;

typedef struct {
	RuleT rule;
	StateT state;
	PreferenceT activePref;
	PreferenceT inactivePref;
	ProximityDirectionT direction;
} ProximityRuleT;

typedef struct {
	RuleT rule;
	uint32_t maximumTime;
    __fp16 usageFloor; //not part of original implementation, but we need to track it here
} UsageRuleT;

typedef struct {
	RuleT rule;
	StateT state;
	PreferenceT activePref;
	PreferenceT inactivePref;
} ScheduleRuleT;

//Context interfaces
bool IsHome(ContextT *c);
void SetIsHome(ContextT *c, bool h);
DayOfWeekT LastUsageDay(ContextT *c);
void SetLastUsageDay(ContextT *c, DayOfWeekT d);
uint32_t TotalTimeUsed(ContextT *c);
void ClearTimeUsed(ContextT *c);
void AddTimeUsed(ContextT *c, uint32_t t);
void UpdateContextCache(ContextT *context, RuleInterfaceT *rules[], int numRules);
RuleResultT GetPreviousResult(ContextT *context, int index);
int FindRuleIDIndex(ContextT *context, uint64_t ruleID);

// Rule logging
void LogStateChange(ContextT *context, uint64_t ruleID, RuleResultT result, bool ruleDeleted, bool overrodeRule);
void LogBlockedRule(ContextT *context, uint64_t ruleID, PreferenceT pref, uint64_t blockingRuleID);

void InitRule(RuleT *rule, uint64_t id, RuleTypeT type, TimelineT timeline);
StateT RunRules(ContextT *context, RuleInterfaceT *rules[], int numRules);
uint32_t secondsSinceMidnight(uint32_t t);
DayOfWeekT dayOfWeek(uint32_t t);
int TestRules(void);
bool IsStrong(PreferenceT p);
bool IsWeak(PreferenceT p);

// Time
bool IsWeekBarrier(TimelineT timeline, int8_t periodIndex);
int8_t InTimeline(TimelineT timeline, uint32_t time);
TimelineT BuildTimeline(DayOfWeekT days, uint32_t start, uint32_t end);

RuleInterfaceT *NewSchedule(uint64_t id, StateT state, TimelineT timeline);
RuleInterfaceT *AutoAway(uint64_t id, StateT state, TimelineT timeline);
RuleInterfaceT *AutoHome(uint64_t id, StateT state, TimelineT timeline);
RuleInterfaceT *UsageLimit(uint64_t id, TimelineT timeline, uint32_t max, uint16_t usage_floor);

void rules_log_init(void);
#endif //_RULES_H

