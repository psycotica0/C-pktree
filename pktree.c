#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "spatial.h"

#define DEFAULT_RANK 3
#define DEFAULT_RATIO_X 2
#define DEFAULT_RATIO_Y DEFAULT_RATIO_X

struct spatial_lookup {
	/* Static properties of the tree */
	/* We have this in every node because there are some techniques that involve alternating stuff */
	int rank;
	int ratio_x;
	int ratio_y;

	/* These are the bounds of this point */
	/* If x1==x2 or y1==y2 then it's considered a point cell */
	int x1, y1;
	int x2, y2;

	/* If this isn't a point cell, this should probably be NULL */
	/* There's no reason it has to be, but it won't be used */
	void *data;

	/* This is the number of children we currently have */
	size_t num_children;
	struct spatial_lookup **children;
};

struct spatial_lookup *internal_new_spatial_lookup(int rank, int ratio_x, int ratio_y, int x1, int y1, int x2, int y2, void *data) {
	struct spatial_lookup *ret = malloc(sizeof(*ret));

	if (ret == NULL) return ret;

	ret->rank = rank;
	ret->ratio_x = ratio_x;
	ret->ratio_y = ratio_y;

	ret->x1 = x1;
	ret->y1 = y1;
	ret->x2 = x2;
	ret->y2 = y2;

	ret->data = data;

	ret->num_children = 0;
	/* Needs to have space for at least (rank) children and at most (rank-1)*ratio_x*ratio_y children */
	/* I'll add (rank) more to this, though, because inbetween stable states there may be as many as rank extra children bound for somewhere else */
	if (x1 == x2 || y1 == y2)
		/* Also, point cells don't have children */
		ret->children = NULL;
	else
		ret->children = calloc((rank-1)*ratio_x*ratio_y + 1, sizeof(*(ret->children)));

	return ret;
}

struct spatial_lookup *new_spatial_lookup(int x1, int y1, int x2, int y2) {
	return internal_new_spatial_lookup(DEFAULT_RANK, DEFAULT_RATIO_X, DEFAULT_RATIO_Y, x1, y1, x2, y2, NULL);
}

void cleanup_spatial_lookup(struct spatial_lookup *tree) {
	size_t i;

	for (i=0; i < tree->num_children; i++) {
		cleanup_spatial_lookup(tree->children[i]);
	}

	/*
	if (tree->children)
		free(tree->children);
	free(tree);
	*/
}

int tree_contains_point(struct spatial_lookup *tree, int x, int y) {
	/* These first two see if this point equals a point cell */
	if ((x == tree->x1) && (x == tree->x2) &&
	    (y == tree->y1) && (y == tree->y2))
		return 1;

	/* Now the range cell */
	if (x > tree->x1 && x <= tree->x2) {
		if (y > tree->y1 && y <= tree->y2) return 1;
	}


	/* Else "no" */
	return 0;
}

/* This function takes a tree */
/* It then returns a new tree which is first undividable cell it could find that contains (rank) or more of this tree's children */
/* If no such subdivision exists, it returns NULL */
struct spatial_lookup *subdivide(struct spatial_lookup *tree, size_t num_children, struct spatial_lookup **children) {
	struct spatial_lookup temp;

	size_t x,y;
	size_t i;

	int width;
	int height;

	width = (tree->x2 - tree->x1) / tree->ratio_x;
	height = (tree->y2 - tree->y1) / tree->ratio_y;

	temp.ratio_x = tree->ratio_x;
	temp.ratio_y = tree->ratio_y;

	for (x=0; x < tree->ratio_x; x++) {
		for (y=0; y < tree->ratio_y; y++) {
			size_t count = 0;

			temp.x1 = (width * x) + tree->x1;
			temp.y1 = (height * y) + tree->y1;
			temp.x2 = temp.x1 + width;
			temp.y2 = temp.y1 + height;

			for (i=0; i < num_children; i++) {
				/* If it's a point, x2 == x1, so I can pick either. */
				/* If it's a subcell, then (x2,y2) is my inclusive edge. */
				/* So, I want that because equal values there are considered in */
				if (tree_contains_point(&temp, children[i]->x2, children[i]->y2)) count++;
			}

			if (count >= tree->rank)
			{
				struct spatial_lookup *ret;
				ret = subdivide(&temp, num_children, children);
				if (ret != NULL)
					return ret;
				return internal_new_spatial_lookup(tree->rank, tree->ratio_x, tree->ratio_y, temp.x1, temp.y1, temp.x2, temp.y2, NULL);
			}
		}
	}

	return NULL;
}

/* This returns a 0 if the tree is K-Intantiable or a 1 if it's not */
int spatial_lookup_insert(struct spatial_lookup *tree, int x, int y, void *data) {
	size_t i;
	/* If I don't insert into myself, and my children are unchanged, then I don't need to check if I'm K-Instantiable */
	int check_required = 0;

	if (tree->x1 == tree->x2 || tree->y1 == tree->y2) {
		tree->data = data;
		return 0;
	}

	/* Look for children that contain the point */
	for(i=0; i < tree->num_children; i++) {
		if (tree_contains_point(tree->children[i], x, y)) {
			if (spatial_lookup_insert(tree->children[i], x, y, data) == 1) {
				struct spatial_lookup *child = tree->children[i];
				size_t subchild;
				check_required = 1;
				/* Move this child's children into me as my children */
				for (subchild=0; subchild < child->num_children; subchild++) {
					tree->children[tree->num_children] = child->children[subchild];
					tree->num_children++;
				}
				child->num_children = 0;
				/* Move my last child into this place in the list so I can delete this child */
				tree->children[i] = tree->children[tree->num_children-1];
				tree->num_children--;
				/* And cleanup this empty child */
				cleanup_spatial_lookup(child);
			}
			break;
		}
	}

	/* Check if we didn't insert it into a child */
	if (i >= tree->num_children) {
		tree->children[tree->num_children] = internal_new_spatial_lookup(DEFAULT_RANK, DEFAULT_RATIO_X, DEFAULT_RATIO_Y, x, y, x, y, data);
		/* XXX: ERROR CHECKING */
		tree->num_children++;
		/* The number of my children has changed */
		check_required = 1;
	}

	/* Now we check to see if we're K-Instantiable or not */
	if (!check_required)
		return 0;

	/* Here, we need to check to see if we're still valid */

	/* Generate subdivisions and see if any of them would contain (rank) or more of my children */
	{
		struct spatial_lookup *subdivision;
		while ((subdivision = subdivide(tree, tree->num_children, tree->children)) != NULL) {
			size_t i;
			/* Move the items into that and then check again */
			for (i=0; i < tree->num_children; i++) {
				if (tree_contains_point(subdivision, tree->children[i]->x2, tree->children[i]->y2)) {
					/* This child is contained inside subdivision */
					subdivision->children[subdivision->num_children] = tree->children[i];
					subdivision->num_children++;
					/* Hack? */
					tree->children[i] = tree->children[tree->num_children-1];
					tree->num_children--;
					i--;
				}
			}
			/* Now link the subdivision as my child */
			tree->children[tree->num_children] = subdivision;
			tree->num_children++;
		}
	}

	/* Once I'm done, check I have fewer than (rank) children. If so, I'm not K-Instantiable */
	if (tree->num_children < tree->rank)
		return 1;
	else
		return 0;
}

size_t spatial_lookup_nearest_k(struct spatial_lookup* tree, int x, int y, int k, void ***data) {
	return 0;
}

size_t spatial_lookup_circle_range(struct spatial_lookup* tree, int cx, int cy, float radius, void ***data) {
	return 0;
}


size_t spatial_lookup_rectangle_range(struct spatial_lookup* tree, int x1, int y1, int x2, int y2, void ***data) {
	return 0;
}

/* This function requires that prefix is a NULL terminated string */
void spatial_debug_output_internal(FILE *file, struct spatial_lookup *tree, char *prefix) {
	char *working;
	size_t len;
	size_t i;

	len = strlen(prefix)+1;
	/* +1 because of the null character */
	working = calloc(len + 1, sizeof *working);
	/* TODO: ERROR CHECKING */

	strcpy(working, prefix);
	strcat(working, "a");

	/* Technically, if this is true then it shouldn't have children, but I'll leave it */
	/* Might help me catch bugs */
	if (tree->x1 == tree->x2 && tree->y1 == tree->y2)
		fprintf(file,"%s [label=\"(%d,%d)\"];\n", prefix, tree->x1, tree->y1);

	for (i=0; i < tree->num_children; i++) {
		/* Draw the link between this node and the child */
		fprintf(file, "%s -> %s;\n", prefix, working);
		/* Then draw the children */
		spatial_debug_output_internal(file, tree->children[i], working);
		/* If we have more than 26 children this could get messy... */
		working[len-1]++;
	}

	free(working);
}

void spatial_debug_output(FILE *file, struct spatial_lookup *tree) {
	fputs("digraph  {\n", file);
	spatial_debug_output_internal(file, tree, "R");
	fputs("}\n", file);
}
