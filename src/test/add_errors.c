#include "sql.h"
#include <time.h>

int main(void) {
	init_database("demo.db.backup");
	remove_all_errors();

	add_error_decoded(0, 22, -1, time(NULL), "Software Error: hi");
	sleep(1);
	add_error_decoded(0, 22, 2, time(NULL), "Hardware Error: Valve 2 exploded");
	add_error_decoded(0, 22, 2, time(NULL), "Hardware Error: More about valve 2");
	sleep(1);
	add_error_decoded(2, 1, -1, time(NULL), "Software Error: another rack");
	sleep(2);
	add_error_decoded(2, 1, -1, time(NULL), "Hardware Error: blah");

	close_database();

	return 0;
}
