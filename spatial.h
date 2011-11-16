#include <stdio.h>
struct spatial_lookup;

/* This function gives you a new opaque structure to use */
/* The universe of points must be contained in the rectangle created by the four points [(x1,y1), (x1,y2), (x2,y2), (x2, y1)] */
struct spatial_lookup *new_spatial_lookup(int x1, int y1, int x2, int y2);
/* This cleans up the structure and frees any memory it allocated */
void cleanup_spatial_lookup(struct spatial_lookup*);

/* This inserts a new opaque bit of data into the set */
/* Currently, only one data can exist at a given (x,y), so this will replace any data that was previously there */
int spatial_lookup_insert(struct spatial_lookup*, int x, int y, void *data);
/* This will remove data at a given point */
int spatial_lookup_delete(struct spatial_lookup*, int x, int y);

/* This function finds the k nearest elements to (x,y) */
/* It returns the number of elements it's found */
/* The pointers of these elements are returned as an array of void pointers with the returned length */
/* You are responsible for freeing this array when you're done with it */
size_t spatial_lookup_nearest_k(struct spatial_lookup*, int x, int y, int k, void ***data);

/* This function finds all elements within radius of (cx, cy) as best as it can given floating point math */
/* Its return is the same as for spatial_lookup_nearest_k */
size_t spatial_lookup_circle_range(struct spatial_lookup*, int cx, int cy, float radius, void ***data);

/* This function returns all elements within the rectangle given by the four points [(x1,y1),(x1,y2),(x2,y2),(x2,y1)] */
size_t spatial_lookup_rectangle_range(struct spatial_lookup*, int x1, int y1, int x2, int y2, void ***data);

/* This function spits out the tree to the given FILE in dot format. */
void spatial_debug_output(FILE *file, struct spatial_lookup*);
