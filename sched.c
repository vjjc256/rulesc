#include <stddef.h>
#include <stdio.h>
#include "rules.h"
#include "log.h"

#define MAX_RULES (6)

static int m_num_rules;
static ScheduleRuleT m_rules[MAX_RULES];
static RuleResultT Run(ContextT *ctx, RuleInterfaceT *rule, uint32_t time, RuleResultT previous);

RuleInterfaceT *NewSchedule(uint64_t id, StateT state, TimelineT timeline)
{
	if (m_num_rules >= MAX_RULES) {
		LOG(LOG_LEVEL_ERROR, "NewSchedule max number of rules!\r\n");
		return NULL;
	}

	InitRule(&m_rules[m_num_rules].rule, id, RuleTypeSchedule, timeline); //init with common interfaces and id, type
	m_rules[m_num_rules].rule.interface.Run = Run;  //our unique run routine
	m_rules[m_num_rules].state = state;

	switch(state) {
		case StateOn:
				m_rules[m_num_rules].activePref = PreferenceStrongOn;
				m_rules[m_num_rules].inactivePref = PreferenceWeakOff;
				break;
		default:				
				m_rules[m_num_rules].activePref = PreferenceStrongOff;
				m_rules[m_num_rules].inactivePref = PreferenceWeakOn;
				break;
	}

	m_rules[m_num_rules].rule.interface.ruleData = &m_rules[m_num_rules];

	return &m_rules[m_num_rules++].rule.interface;
}

static RuleResultT Run(ContextT *ctx, RuleInterfaceT *rule, uint32_t time, RuleResultT previous)
{
	ScheduleRuleT *_rule = rule->ruleData;

	RuleResultT result;
	result.DeletedPreference = _rule->inactivePref;
	result.PeriodIndex = InTimeline(_rule->rule.timeline, time);
	result.WeekBarrier = IsWeekBarrier(_rule->rule.timeline, result.PeriodIndex);
	result.DisableOverride = false;
	result.LogUsage = false;
	result.ClearOverride = result.PeriodIndex != previous.PeriodIndex && !previous.WeekBarrier;

	if (result.PeriodIndex >= 0) {
		result.Preference = _rule->activePref;
	} else {
		result.Preference = _rule->inactivePref;
	}
	return result;
}
