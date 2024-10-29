#define assert(condition) do { if(!(condition)) verbose_abort("assertion failed : '" #condition "'"); } while(0)
#define verbose_abort(message) do { MessageBoxA(NULL, "'" message "'", "simple_tray_program crashed!", MB_OK | MB_ICONERROR | MB_TOPMOST); abort(); } while(0)
