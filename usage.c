#include <stddef.h>
#include <stdio.h>
#include "log.h"
#include "rules.h"
#include "meter.h"

#define MAX_RULES (3)

static int m_num_rules;
static UsageRuleT m_rules[MAX_RULES];
static RuleResultT Run(ContextT *ctx, RuleInterfaceT *rule, uint32_t time, RuleResultT previous);

RuleInterfaceT *UsageLimit(uint64_t id, TimelineT timeline, uint32_t max, uint16_t usage_floor)
{
	if (m_num_rules >= MAX_RULES) {
		LOG(LOG_LEVEL_ERROR, "UsageLimit max number of rules!\r\n");
		return NULL;
	}

	InitRule(&m_rules[m_num_rules].rule, id, RuleTypeSchedule, timeline); //init with common interfaces and id, type
	m_rules[m_num_rules].rule.interface.Run = Run;  //our unique run routine

	m_rules[m_num_rules].maximumTime = max;
	m_rules[m_num_rules].usageFloor = usage_floor;


	m_rules[m_num_rules].rule.interface.ruleData = &m_rules[m_num_rules];

	return &m_rules[m_num_rules++].rule.interface;
}

static RuleResultT Run(ContextT *ctx, RuleInterfaceT *rule, uint32_t time, RuleResultT previous)
{
	UsageRuleT *_rule = rule->ruleData;

	RuleResultT result;
	result.DeletedPreference = PreferenceWeakOn;
	result.ClearOverride = false;
	result.DisableOverride = false;
	result.LogUsage = false;

	result.PeriodIndex = InTimeline(_rule->rule.timeline, time);
	result.WeekBarrier = IsWeekBarrier(_rule->rule.timeline, result.PeriodIndex);

	if (previous.PeriodIndex != result.PeriodIndex && !previous.WeekBarrier) {
		ClearTimeUsed(ctx);
	}

	if (result.PeriodIndex >= 0) {

		if (TotalTimeUsed(ctx) >= _rule->maximumTime) {
			result.Preference = PreferenceStrongOff;
			result.DisableOverride = true;
		} else {
			result.Preference = PreferenceWeakOn;
		}
		
		float w = meter_get_active_power(ctx->relay);

		// If we're above the usage floor, log usage
        if (w >= _rule->usageFloor) {
			result.LogUsage = true;
        }

	} else {
		//We're out of the measurement zone, so clear our counter
		ClearTimeUsed(ctx);
		result.Preference = PreferenceWeakOn;
	}
	return result;
}
