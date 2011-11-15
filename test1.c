#include <stdio.h>
#include <stdlib.h>
#include "spatial.h"

int main(int argc, char **argv) {
	size_t i;
	void **array;
	struct spatial_lookup *look = new_spatial_lookup(0, 0, 16, 16);

	char *strings[]= {"1", "2", "3", "4", "5", "6", "7"};
	int xs[] = {2, 13, 2, 11, 11, 9, 11};
	int ys[] = {2, 5, 13, 5, 7, 6, 1};

	puts("Inserting");
	for (i=0; i < sizeof(strings); i++) {
		spatial_lookup_insert(look, xs[i], ys[i], strings[i]);
	}

	puts("Nearest K:");
	{
		size_t total = spatial_lookup_nearest_k(look, 13, 6, 2, &array);
		for (i=0; i < total; i++) {
			puts((char *)array[i]);
		}
	}

	puts("Circle Range:");
	{
		size_t total = spatial_lookup_circle_range(look, 12, 6, 5, &array);
		for (i=0; i < total; i++) {
			puts((char *)array[i]);
		}
	}

	puts("Rectangle Range:");
	{
		size_t total = spatial_lookup_rectangle_range(look, 1, 1, 4, 15, &array);
		for (i=0; i < total; i++) {
			puts((char *)array[i]);
		}
	}

	cleanup_spatial_lookup(look);
}
