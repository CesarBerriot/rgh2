#include "utils.h"
#include <windows.h>
#include "macros/macros.h"

void single_instance_check(void)
{	char mutex_id[] = "{AC293D3F-1CF4-4B73-9372-DE132230D142}";
	SetLastError(0);
	if(OpenMutexA(SYNCHRONIZE, FALSE, mutex_id))
		verbose_abort("an instance of Replay Glitch Helper 2 already exists!");
	else
		assert(GetLastError() == ERROR_FILE_NOT_FOUND);
	assert(CreateMutexA(NULL, TRUE, mutex_id));
}

bool is_elevated(void)
{	HANDLE process_access_token;
	assert(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &process_access_token));
	TOKEN_ELEVATION elevation_status;
	DWORD size = sizeof(TOKEN_ELEVATION);
	assert(GetTokenInformation(process_access_token, TokenElevation, &elevation_status, size, &size));
	assert(CloseHandle(process_access_token));
	return elevation_status.TokenIsElevated;
}

void restart_elevated(void)
{	char path[MAX_PATH + 1];
	DWORD size = MAX_PATH;
	assert(QueryFullProcessImageNameA(GetCurrentProcess(), 0, path, &size));
	assert((INT_PTR)ShellExecuteA(NULL, "runas", path, "", NULL, SW_SHOWNORMAL) > 32);
	exit(EXIT_SUCCESS);
}
