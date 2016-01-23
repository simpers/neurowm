//----------------------------------------------------------------------------------------------------------------------
// Module      :  workspace
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

// Includes
#include "workspace.h"
#include "system.h"
#include "config.h"
#include "core.h"
#include "layout.h"
#include "client.h"
#include "rule.h"


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE VARIABLE DECLARATION
//----------------------------------------------------------------------------------------------------------------------

// ClientFn
typedef void (*WorkspaceClientFn)(ClientPtrPtr c, const void *data);


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

static bool is_above_tiled_client(const ClientPtrPtr c) {
  assert(c);
  return (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull) || CLI_GET(c).is_fullscreen;
}

static void focus_client(ClientPtrPtr c) {
  assert(c);
  NeuroClientUnsetUrgent(c, NULL);
  NeuroSystemUngrabButtons(CLI_GET(c).win, NeuroConfigGet()->button_list);
  XSetInputFocus(NeuroSystemGetDisplay(), CLI_GET(c).win, RevertToPointerRoot, CurrentTime);
  XChangeProperty(NeuroSystemGetDisplay(), NeuroSystemGetRoot(), NeuroSystemGetNetAtom(NeuroSystemNetAtomActive),
      XA_WINDOW, 32, PropModeReplace, (unsigned char *)&(CLI_GET(c).win), 1);
}

static void unfocus_client(ClientPtrPtr c) {
  assert(c);
  NeuroSystemGrabButtons(CLI_GET(c).win, NeuroConfigGet()->button_list);
}

static void process_client(const WorkspaceClientFn wcf, const ClientPtrPtr ref, const ClientSelectorFn csf,
    const void *data) {
  if (!ref || !csf)
    return;
  ClientPtrPtr c = csf(ref);
  if (!c)
    return;
  wcf(c, data);
}

static void send_client(ClientPtrPtr c, const void *data) {
  if (!c || !data)
    return;

  // Get the workspaces
  const size_t new_ws = *(const size_t *)data;
  const size_t old_ws = CLI_GET(c).ws;
  const size_t curr_ws = NeuroCoreGetCurrStack();
  if (old_ws == new_ws)
    return;

  // 'Default free' windows are set to 'Center free' because of different screen resolutions
  if (CLI_GET(c).free_setter_fn == NeuroRuleFreeSetterDefault)
    CLI_GET(c).free_setter_fn = NeuroRuleFreeSetterCenter;

  // Remove the client from its stack
  Client *const cli = NeuroCoreRemoveClient(c);

  // Add the client to the new stack
  cli->ws = new_ws;
  const ClientPtrPtr c2 = NeuroCoreAddClientStart(cli);
  if (!c2)
    NeuroSystemError("send_client - Could not add client");

  // Update old and new workspaces
  NeuroLayoutRunCurr(curr_ws);
  NeuroWorkspaceUpdate(curr_ws);
  NeuroLayoutRunCurr(new_ws);
  NeuroWorkspaceUpdate(new_ws);

  // Focus the current workspace
  NeuroWorkspaceFocus(curr_ws);
}


//----------------------------------------------------------------------------------------------------------------------
// PUBLIC FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

void NeuroWorkspaceChange(size_t ws) {
  if (NeuroCoreStackIsCurr(ws))
    return;

  const size_t curr = NeuroCoreGetCurrStack();
  const Monitor *const curr_monitor = NeuroCoreStackGetMonitor(curr);
  const Monitor *const new_monitor = NeuroCoreStackGetMonitor(ws);
  NeuroCoreStackSetMonitor(curr, new_monitor);
  NeuroCoreStackSetMonitor(ws, curr_monitor);

  // Update old and new workspaces
  NeuroLayoutRunCurr(curr);
  NeuroWorkspaceUpdate(curr);
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceUpdate(ws);

  // Focus the current workspace
  NeuroWorkspaceUnfocus(curr);
  NeuroCoreSetCurrStack(ws);
  NeuroWorkspaceFocus(ws);
}

void NeuroWorkspaceUpdate(size_t ws) {
  for (ClientPtrPtr c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    NeuroClientUpdate(c, NULL);
}

void NeuroWorkspaceFocus(size_t ws) {
  size_t n = NeuroCoreStackGetSize(ws);
  if (n <= 0) {
    XDeleteProperty(NeuroSystemGetDisplay(), NeuroSystemGetRoot(), NeuroSystemGetNetAtom(NeuroSystemNetAtomActive));
    return;
  }

  Window windows[ n ], d1, d2, *wins = NULL;
  size_t atc = 0U;
  ClientPtrPtr c = NeuroCoreStackGetHeadClient(ws);
  for ( ; c; c = NeuroCoreClientGetNext(c))
    if (is_above_tiled_client(c))
      ++atc;

  c = NeuroCoreStackGetCurrClient(ws);
  windows[ is_above_tiled_client(c) ? 0U : atc ] = CLI_GET(c).win;
  focus_client(c);
  NeuroClientUpdate(c, NULL);

  if (n > 1) {
    // XQueryTree gets windows by stacking order
    unsigned int num = 0U;
    if (!XQueryTree(NeuroSystemGetDisplay(), NeuroSystemGetRoot(), &d1, &d2, &wins, &num))
      NeuroSystemError("NeuroWorkspaceFocus - Could not get windows");
    size_t n2 = n;
    for (unsigned int i = 0U; i < num; ++i) {
      ClientPtrPtr c = NeuroWorkspaceClientFindWindow(ws, wins[ i ]);
      if (!c)
        continue;
      if (NeuroCoreClientIsCurr(c))
        continue;
      windows[ is_above_tiled_client(c) ? --atc : --n2 ] = CLI_GET(c).win;
      unfocus_client(c);
      NeuroClientUpdate(c, NULL);
    }
    if (wins)
      XFree(wins);
  }

  XRestackWindows(NeuroSystemGetDisplay(), windows, n);
}

void NeuroWorkspaceUnfocus(size_t ws) {
  for (ClientPtrPtr c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    unfocus_client(c);
}

void NeuroWorkspaceTile(size_t ws) {
  for (ClientPtrPtr c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    NeuroClientTile(c, NULL);
}

void NeuroWorkspaceFree(size_t ws, const void *free_setter_fn) {
  for (ClientPtrPtr c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    NeuroClientFree(c, free_setter_fn);
}

void NeuroWorkspaceMinimize(size_t ws) {
  for (ClientPtrPtr c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    NeuroClientMinimize(c, NULL);
}

void NeuroWorkspaceRestoreLastMinimized(size_t ws) {
  if (NeuroCoreStackGetMinimizedNum(ws) <= 0)
    return;
  Client *cli = NeuroCorePopMinimizedClient(ws);
  if (!cli)
    NeuroSystemError("NeuroWorkspaceRestoreLastMinimized - Could not restore last minimized client");
  ClientPtrPtr c = NeuroCoreAddClientStart(cli);
  if (!c)
    NeuroSystemError("NeuroWorkspaceRestoreLastMinimized - Could not add client");
  NeuroCoreSetCurrClient(c);
  NeuroLayoutRunCurr(CLI_GET(c).ws);
  NeuroWorkspaceFocus(CLI_GET(c).ws);
}

void NeuroWorkspaceAddEnterNotifyMask(size_t ws) {
  ClientPtrPtr c;
  for (c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    XSelectInput(NeuroSystemGetDisplay(), CLI_GET(c).win, CLIENT_MASK);
}

void NeuroWorkspaceRemoveEnterNotifyMask(size_t ws) {
  ClientPtrPtr c;
  for (c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c))
    XSelectInput(NeuroSystemGetDisplay(), CLI_GET(c).win, CLIENT_MASK_NO_ENTER);
}

// Find functions
ClientPtrPtr NeuroWorkspaceClientFindWindow(size_t ws, Window w) {
  return NeuroCoreStackFindClient(ws, NeuroClientTesterWindow, (const void *)&w);
}

ClientPtrPtr NeuroWorkspaceClientFindUrgent(size_t ws) {
  return NeuroCoreStackFindClient(ws, NeuroClientTesterUrgent, NULL);
}

ClientPtrPtr NeuroWorkspaceClientFindFixed(size_t ws) {
  return NeuroCoreStackFindClient(ws, NeuroClientTesterFixed, NULL);
}

// Clients
void NeuroWorkspaceClientFocus(const ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  (void)data;
  if (!ref || !csf)
    return;
  ClientPtrPtr c = csf(ref);
  if (!c)
    return;
  const size_t ws = CLI_GET(c).ws;
  // NeuroCoreSetCurrStack(ws);  // This focuses new windows in another workspaces
  NeuroCoreSetCurrClient(c);
  NeuroWorkspaceFocus(ws);
}

void NeuroWorkspaceClientSwap(const ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  (void)data;
  if (!ref || !csf)
    return;
  ClientPtrPtr c = csf(ref);
  if (!c)
    return;
  if (!NeuroCoreClientSwap(ref, c))
    return;
  NeuroWorkspaceUpdate(CLI_GET(c).ws);
  NeuroWorkspaceClientFocus(ref, csf, NULL);
}

void NeuroWorkspaceClientSend(ClientPtrPtr ref, const ClientSelectorFn csf, const void *ws) {
  process_client(send_client, ref, csf, ws);
}

void NeuroWorkspaceClientKill(const ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientKill, ref, csf, data);
}

void NeuroWorkspaceClientMinimize(const ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientMinimize, ref, csf, data);
}

void NeuroWorkspaceClientTile(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientTile, ref, csf, data);
}

void NeuroWorkspaceClientFree(ClientPtrPtr ref, const ClientSelectorFn csf, const void *freeSetterFn) {
  process_client(NeuroClientFree, ref, csf, freeSetterFn);
}

void NeuroWorkspaceClientToggleFree(ClientPtrPtr ref, const ClientSelectorFn csf, const void *free_setter_fn) {
  process_client(NeuroClientToggleFree, ref, csf, free_setter_fn);
}

void NeuroWorkspaceClientNormal(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientNormal, ref, csf, data);
}

void NeuroWorkspaceClientFullscreen(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientFullscreen, ref, csf, data);
}

void NeuroWorkspaceClientToggleFullscreen(const ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientToggleFullscreen, ref, csf, data);
}

void NeuroWorkspaceClientFloatMove(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientFloatMove, ref, csf, data);
}

void NeuroWorkspaceClientFloatResize(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientFloatResize, ref, csf, data);
}

void NeuroWorkspaceClientFreeMove(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientFreeMove, ref, csf, data);
}

void NeuroWorkspaceClientFreeResize(ClientPtrPtr ref, const ClientSelectorFn csf, const void *data) {
  process_client(NeuroClientFreeResize, ref, csf, data);
}

// Workspace Selectors
size_t NeuroWorkspaceSelector0() {
  return 0;
}

size_t NeuroWorkspaceSelector1() {
  return 1;
}

size_t NeuroWorkspaceSelector2() {
  return 2;
}

size_t NeuroWorkspaceSelector3() {
  return 3;
}

size_t NeuroWorkspaceSelector4() {
  return 4;
}

size_t NeuroWorkspaceSelector5() {
  return 5;
}

size_t NeuroWorkspaceSelector6() {
  return 6;
}

size_t NeuroWorkspaceSelector7() {
  return 7;
}

size_t NeuroWorkspaceSelector8() {
  return 8;
}

size_t NeuroWorkspaceSelector9() {
  return 9;
}

size_t NeuroWorkspaceSelectorCurr() {
  return NeuroCoreGetCurrStack();
}

size_t NeuroWorkspaceSelectorPrev() {
  return NeuroCoreGetPrevStack();
}

size_t NeuroWorkspaceSelectorNext() {
  return NeuroCoreGetNextStack();
}

size_t NeuroWorkspaceSelectorOld() {
  return NeuroCoreGetOldStack();
}

