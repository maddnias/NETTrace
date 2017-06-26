#include "stdafx.h"
#include "Constants.h"

const wchar_t *hostPipeEnvName = L"NETTrace_pipe_name";
const char *settingsFileName = "NETTrace_settings_file";
const int maxTransferSize = 65536;
const int pipeWaitTimeout = 10000 /* ms */;
const int maxPipeCount = 100;
const int mdMemberMaxLength = 255;