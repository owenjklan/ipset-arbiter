#include <sys/sysinfo.h>

int cpu_count()
{
	return get_nprocs();
}