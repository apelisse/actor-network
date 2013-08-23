#ifndef __PACK_H_
#define __PACK_H_

#include <glib.h>

struct node {
	/* Database specific fields */
	guint32 *id;
	const char *name;
	enum type { kPerson, kMovie, kSeries, kTV, kVideo, kGame } type;
	guint32 year;
	GArray *roles;
	guint32 _rolesposition;

	/* Algorithm fields */
	struct node *from;
};

struct database {
	void *addr;
	guint32 numnodes;
	struct node **nodes;	/* These are already loaded nodes */
	guint32 numactors;
	guint32 *actors;
};

struct database *open_database(char *name);

/* Get node from database at given position */
struct node *get_node(struct database *database, guint32 position);
/* Get nodes for each roles of node */
void get_roles(struct database *database, struct node *node);

/* Find actor in index with given name */
struct node *find_actor(struct database *database, const gchar *name);

#endif
