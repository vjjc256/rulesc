#include "rules.h"
#include <string.h>

bool IsHome(ContextT *c) 
{
	return c->home;
}

void SetIsHome(ContextT *c, bool h)
{
	c->home = h;
}

uint32_t TotalTimeUsed(ContextT *c)
{
	return c->totalUsed;
}

void ClearTimeUsed(ContextT *c)
{
	c->totalUsed = 0;
}

void AddTimeUsed(ContextT *c, uint32_t t)
{
	c->totalUsed += t;
}

RuleResultT GetPreviousResult(ContextT *c, int index) {
	return c->RuleResults[index];
}

void UpdateRuleCache(ContextT *context, RuleInterfaceT *rules[], int numRules) {
	RuleResultT newPrevResults[MAX_NUMBER_OF_RULES];
	uint64_t    newRuleIDIndex[MAX_NUMBER_OF_RULES];
	memset(newPrevResults, 0, sizeof(RuleResultT) * MAX_NUMBER_OF_RULES);
	memset(newRuleIDIndex, 0, sizeof(uint64_t) * MAX_NUMBER_OF_RULES);
	for (int i = 0; i<numRules; i++) {
		RuleInterfaceT *rule = rules[i];
		int lastStateIndex = -1;
		for (int j = 0; j<MAX_NUMBER_OF_RULES; j++) {
			uint64_t id = context->RuleIDIndex[j];
			if (id == rule->Id(rule)) {
				lastStateIndex = j;
			}
			if (lastStateIndex >= 0) {
				newPrevResults[i].Preference = context->RuleResults[lastStateIndex].Preference;
				newPrevResults[i].DeletedPreference = context->RuleResults[lastStateIndex].DeletedPreference;
				newPrevResults[i].PeriodIndex = -1;
				newPrevResults[i].DisableOverride = context->RuleResults[lastStateIndex].DisableOverride;
				newPrevResults[i].ClearOverride = context->RuleResults[lastStateIndex].ClearOverride;
			}
			newRuleIDIndex[i] = rule->Id(rule);
		}
	}
	memcpy(context->RuleResults, newPrevResults, sizeof(RuleResultT) * MAX_NUMBER_OF_RULES);
	memcpy(context->RuleIDIndex, newRuleIDIndex, sizeof(uint64_t) * MAX_NUMBER_OF_RULES);
}

int FindRuleIDIndex(ContextT *context, uint64_t ruleID) {
	for (int i = 0; i<MAX_NUMBER_OF_RULES; i++) {
		if (context->RuleIDIndex[i] == ruleID) {
			return i;
		}
	}
	return -1;
}