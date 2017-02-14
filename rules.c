#include <stddef.h>
#include "rules.h"
#include "log.h"
#include <string.h>

/*
 * Routines common to all Rule types
 */
static uint64_t Id(RuleInterfaceT *rule)
{
	RuleT *_rule = (RuleT *)rule->ruleData;
	return _rule->id;
}

static RuleTypeT Type(RuleInterfaceT *rule)
{
	RuleT *_rule = (RuleT *)rule->ruleData;
	return _rule->ruleType;
}

/*
 * Initialize a rule, called when creating a new rule
 */

void InitRule(RuleT *rule, uint64_t id, RuleTypeT type, TimelineT timeline)
{
	rule->id = id;
	rule->ruleType = type;
	rule->timeline = timeline;

	RuleInterfaceT *interface = &rule->interface;
	interface->Id = Id;
	interface->Type = Type;
}

bool IsStrong(PreferenceT p) {
        return (p == PreferenceStrongOn || p == PreferenceStrongOff);
}

bool IsWeak(PreferenceT p) {
        return (p == PreferenceWeakOn || p == PreferenceWeakOff);
}


/*
 * RunRules - this is the meat of the rules engine.  It iterates over all
 * the rules in the rules[] array, gets the preference for each rule based
 * on the current time and home/away state.  It sets the context state and
 * returns the current state.
 */
StateT RunRules(ContextT *c, RuleInterfaceT *rules[], int numRules)
{
	uint64_t operativeRuleID = 0;
	int operativeRuleIndex = -1;
	RuleResultT operativeResult;
	bool operativeRuleDeleted = false;
	bool logUsage = false; // We'll use this waaaay down at the bottom of the function, if any rules ask to log usage

	// Run all our rules, store the results in an ordered array
	RuleResultT results[MAX_NUMBER_OF_RULES];
	memset((uint8_t *)&operativeResult, 0, sizeof(operativeResult));
	memset(results, 0, sizeof(RuleResultT) * MAX_NUMBER_OF_RULES); 
	for (int i = 0; i<numRules; i++) {
		RuleInterfaceT *rule = rules[i];
		if (rule->target != COIL_BOTH && rule->target != c->relay) continue; //not applicable for this relay
		RuleResultT result = rule->Run(c, rule, c->now, GetPreviousResult(c, i));
		if (IsStrong(result.Preference) && operativeRuleIndex == -1) {
			operativeRuleIndex = i;
		}
		results[i] = result;
		logUsage = logUsage || result.LogUsage;
	}

	// We didn't find a rule with a strong preference
	if (operativeRuleIndex == -1) {
		// Let's find the least important rule with a weak preference
		for (int i = 0; i<numRules; i++) {
			RuleResultT result = results[i];
			if (result.Preference == PreferenceNone) continue;
			if (IsWeak(result.Preference)) {
				operativeRuleIndex = i;
				if (IsStrong(c->RuleResult.Preference)){
					RuleInterfaceT *rule = rules[operativeRuleIndex];
					if (c->RuleID == rule->Id(rule)) { // This used to be the operative rule, and it had a strong preference that's now weak
						// We prefer this to be the operative rule, for logging purposes
						break;
					}
				}
			}
		}
	}
	
	if (operativeRuleIndex >= 0) {
		RuleInterfaceT *rule = rules[operativeRuleIndex];
		operativeRuleID = rule->Id(rule);
		operativeResult = results[operativeRuleIndex];
	} else if (c->RuleID > 0 && IsStrong(c->RuleResult.Preference) && FindRuleIDIndex(c, c->RuleID) == -1) {
		// Our last operative rule was deleted while it was strong, and no one else has an opinion
		operativeRuleID = c->RuleID;
		operativeResult = c->RuleResult;
		operativeRuleDeleted = true;

		// We'll switch to whatever the last rule wanted us to do when it was deleted
		operativeResult.Preference = operativeResult.DeletedPreference;
	}

	// Check to make sure we didn't change rule prefs too recently;
	// this prevents thrashing on time updates
	if (operativeResult.Preference != c->RuleResult.Preference && c->LastRulePrefChange > 0) {
		int elapsed = c->now - c->LastRulePrefChange;
		if (elapsed < 0) elapsed = -elapsed;
		if (elapsed < 60*Second) {
			if (logUsage) {
				// TODO: log usage
			}
			return c->state;  //return current state
		}
	}

	// Check to see if we should clear a manual override
	// We only clear if there's currently an operative rule, or if the deleted rule had a strong preference
	if (c->override != OverrideNone && (c->RuleID > 0 || IsStrong(operativeResult.Preference))) {
		bool clearOverride = false;
		if (c->RuleID == 0) {
			// A rule with a strong preference disappeared, so clear the override
			clearOverride = true;
		} else if (c->RuleID != operativeRuleID) {
			// If we've switched rules, the manual override must've applied to the previous rule
			clearOverride = true;
		} else if (c->RuleID == operativeRuleID) {
			if (operativeResult.Preference != c->RuleResult.Preference) {
				// It's the same rule, but it's changed its preference
				clearOverride = true;
			} else if (operativeResult.ClearOverride) {
				// Everything's the same, but the rule thinks we should clear it
				clearOverride = true;
			}
		}
		if (clearOverride) {
			c->override = OverrideNone;
		}
	}


	StateT newState = c->state;
	bool overrodeRule = false;
	if (c->override != OverrideNone &&  // We're currently overridden
		(operativeRuleID == 0 // There are no rules in effect, so process manual override, or...
			|| !operativeResult.DisableOverride  // There is an effective rule, but it hasn't disabled manual override, or...
			|| IsWeak(operativeResult.Preference))) { // The effective rule disables manual override, but it doesn't have a strong opinion
			switch (c->override) {
				case OverrideOn:
				{
					newState = StateOn;
					overrodeRule = true;
					break;
				}
				case OverrideOff:
				{
					newState = StateOff;
					overrodeRule = true;
					break;
				}
				default:
					break;
			}
	} else {
		switch (operativeResult.Preference) {
			case PreferenceWeakOn:
			case PreferenceStrongOn:
				//LOG(LOG_LEVEL_DEBUG, "preference to on\r\n");
				newState = StateOn;
				break;
			case PreferenceWeakOff:
			case PreferenceStrongOff:
				//LOG(LOG_LEVEL_DEBUG, "preference to off\r\n");
				newState = StateOff;
				break;
		default:
			// No one has any preference, so leave things alone
			break;
		}
	}


	bool stateChange = (newState != c->state);
	if (stateChange) {
		c->state = newState;
		LogStateChange(c, operativeRuleID, operativeResult, operativeRuleDeleted, overrodeRule);
	}

	if (operativeResult.Preference != c->RuleResult.Preference) {
		c->LastRulePrefChange = c->now;
	}

	// Actually set the current rule ID
	c->RuleID = operativeRuleID;
	c->RuleResult = operativeResult;

	if (operativeResult.Preference != c->RuleResult.Preference) { // The rule-based pref is different than it used to be
		c->LastRulePrefChange = c->now;
	}

	for (int i = 0; i<MAX_NUMBER_OF_RULES; i++) {
			// Log overridden rules
			RuleResultT result = results[i];
			PreferenceT prev = c->RuleResults[i].Preference;
			if (result.Preference != prev && // If the result's preference changed, and...
				result.Preference != operativeResult.Preference && // it has a different preference than operative rule, and...
				IsStrong(result.Preference) && // it has a strong preference, and...
				IsWeak(prev)) { // its previous preference was weak...
				//...then it was overridden by the operative rule; log it
				RuleInterfaceT *blockedRule = rules[i];
				LogBlockedRule(c, blockedRule->Id(blockedRule), result.Preference, operativeRuleID);
			}
	}
	memset(c->RuleResults, 0, sizeof(RuleResultT) * MAX_NUMBER_OF_RULES);
	memcpy(c->RuleResults, results, sizeof(RuleResultT) * MAX_NUMBER_OF_RULES);
	if (logUsage) {
		AddTimeUsed(c, 1);
	}
	return c->state;
}

uint32_t secondsSinceMidnight(uint32_t t)
{
	return t % (24 * Hour);
}

DayOfWeekT dayOfWeek(uint32_t t)
{
	return 1 << (uint8_t)(t/(24*Hour));
}

bool InTimespan(uint32_t time, DayOfWeekT dayOfWeek, uint32_t start, uint32_t end, DayOfWeekT days, DayOfWeekT *ruleDay)
{
	if (start < end) {
		if (start <= time && end > time && (dayOfWeek & days) == dayOfWeek) {
			*ruleDay = dayOfWeek;
			return true;
		}
	} else if (start > end) { // This schedule passes midnight
		if ((dayOfWeek & days) == dayOfWeek && start <= time) {
			*ruleDay = dayOfWeek;
			return true;
		}
		DayOfWeekT prevDay = dayOfWeek >> 1;
		if (prevDay == 0) {
			prevDay = Saturday;
		}
		if ((prevDay & days) == prevDay && end > time) {
			*ruleDay = prevDay;
			return true;
		}
	} else { // start == end, so all day
		if ((dayOfWeek & days) == dayOfWeek) {
			*ruleDay = dayOfWeek;
			return true;
		}
	}
	*ruleDay = dayOfWeek;
	return false;
}
