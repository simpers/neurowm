//----------------------------------------------------------------------------------------------------------------------
// Module      :  rule
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

#pragma once

// Includes
#include "type.h"

// Defines
#define RULE_SCRATCHPAD_NAME "neurowm_scratchpad"
#define NeuroRuleFreeSetterNull NULL


//----------------------------------------------------------------------------------------------------------------------
// FUNCTION DECLARATION
//----------------------------------------------------------------------------------------------------------------------

// Basic Functions
Client *NeuroRuleNewClient(Window w, const XWindowAttributes *wa);
void NeuroRuleSetLayoutRegion(Rectangle *r, const ClientPtrPtr c);
void NeuroRuleSetClientRegion(Rectangle *r, const ClientPtrPtr c);

// Free Setters
void NeuroRuleFreeSetterDefault(Rectangle *a, const Rectangle *r);
void NeuroRuleFreeSetterCenter(Rectangle *a, const Rectangle *r);
void NeuroRuleFreeSetterBigCenter(Rectangle *a, const Rectangle *r);
void NeuroRuleFreeSetterScratchpad(Rectangle *a, const Rectangle *r);

