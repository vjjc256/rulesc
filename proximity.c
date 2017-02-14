#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "rules.h"
#include "log.h"


#define MAX_RULES (3)

static int m_num_rules;
static ProximityRuleT m_rules[MAX_RULES];
static RuleInterfaceT *newProximity(uint64_t id, ProximityDirectionT direction, StateT state, TimelineT timeline);
static RuleResultT Run(ContextT *ctx, RuleInterfaceT *rule, uint32_t time, RuleResultT previous);

RuleInterfaceT *AutoAway(uint64_t id, StateT state, TimelineT timeline)
{
	return newProximity(id, proximityDirectionAway, state, timeline);
}

RuleInterfaceT *AutoHome(uint64_t id, StateT state, TimelineT timeline)
{
	return newProximity(id, proximityDirectionHome, state, timeline);
}


static RuleInterfaceT *newProximity(uint64_t id, ProximityDirectionT direction, StateT state, TimelineT timeline)
{
	if (m_num_rules >= MAX_RULES) {
		LOG(LOG_LEVEL_ERROR, "newProximity max number of rules!\r\n");
		return NULL;
	}

	InitRule(&m_rules[m_num_rules].rule, id, RuleTypeProximity, timeline); //init with common interfaces and id, type
	m_rules[m_num_rules].rule.interface.Run = Run;  //our unique run routine

	m_rules[m_num_rules].direction = direction;
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
	ProximityRuleT *_rule = rule->ruleData;
	RuleResultT result;
	result.DeletedPreference = _rule->inactivePref;
	result.PeriodIndex = InTimeline(_rule->rule.timeline, time);
	result.WeekBarrier = IsWeekBarrier(_rule->rule.timeline, result.PeriodIndex);
	result.Preference = _rule->inactivePref;
	result.ClearOverride = false;
	result.DisableOverride = false;
	result.LogUsage = false;

	if (result.PeriodIndex >= 0) {
		switch (_rule->direction) {
			case proximityDirectionHome:
				if (IsHome(ctx)) {
					result.Preference = _rule->activePref;
				} else {
					result.Preference = _rule->inactivePref;
				}
			break;

			default: //AutoAway
				if (IsHome(ctx)) {
					result.Preference =  _rule->inactivePref;
				} else {
					result.Preference =  _rule->activePref;
				}
			break;

		}
		result.ClearOverride = previous.Preference != result.Preference;
	}
	return result;
}
