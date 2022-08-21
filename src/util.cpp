#include "quickshare.hpp"

void _colored_print(void* color, const char* str, ...)
{
    va_list ap;
	va_start(ap, str);

#if defined(SYSTEM_WIN_64)
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (INT_PTR)color);
#elif defined(SYSTEM_UNX)
    std::fprintf(stdout, "%s", (char*)color);
#endif
	std::vfprintf(stdout, str, ap);
#if defined(SYSTEM_WIN_64)
    SetConsoleTextAttribute(hConsole, CL_RESET);
#elif defined(SYSTEM_UNX)
    std::fprintf(stdout, "%s", CL_RESET);
#endif

	va_end(ap);
}
