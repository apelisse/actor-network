#include "pack.h"

#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GET_POINTER(db, off, type) ((type)((char *)db + off))
#define GET_VALUE(db, off, type) (*GET_POINTER(db, off, type *))
#define ALIGN(off) ((off + 3) & ~3)

static void
read_index(struct database *database)
{
	guint32 index_offset;

	index_offset = GET_VALUE(database->addr, 0, guint32);
	database->numactors = GET_VALUE(database->addr, index_offset, guint32);
	database->actors = GET_POINTER(database->addr,
				       index_offset + sizeof(database->numactors),
				       guint32*);
}

struct database *
open_database(char *name)
{
	struct database *database;
	struct stat sb;
	int fd;

	fd = open(name, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	if (fstat(fd, &sb) == -1) {
		perror("fstat");
		exit(1);
	}

	database = g_new(struct database, 1);

	database->addr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	if (database->addr == NULL) {
		perror("mmap");
		exit(1);
	}

	database->numnodes = GET_VALUE(database->addr, 4, guint32);
	database->nodes = g_new0(struct node *, database->numnodes);

	read_index(database);

	return database;
}

/* Get node from database at given position */
struct node *
get_node(struct database *database, guint32 position)
{
	struct node *node;
	size_t namelen;
	guint32 numroles;
	guint32 *id;

	id = GET_POINTER(database->addr, position, guint32 *);

	if (database->nodes[*id] != NULL)
		return database->nodes[*id];

	node = g_slice_new(struct node);
	node->id = id;
	position += sizeof(*node->id);
	node->name = GET_POINTER(database->addr, position, const char *);
	namelen = strlen(node->name);
	node->type = GET_VALUE(database->addr, position + namelen + 1, guint8);
	position += ALIGN(namelen + 2);
	if (node->type != kPerson) {
		node->year = GET_VALUE(database->addr, position, guint32);
		position += sizeof(guint32);
	} else
		node->year = G_MAXUINT32;

	numroles = GET_VALUE(database->addr, position, guint32);
	node->roles = NULL;
	node->_rolesposition = position;

	node->from = NULL;

	database->nodes[*node->id] = node;

	return node;
}

void
get_roles(struct database *database, struct node *node)
{
	guint32 i, numroles;

	if (node->roles != NULL)
		return;

	numroles = GET_VALUE(database->addr, node->_rolesposition, guint32);

	node->roles = g_array_sized_new(FALSE, FALSE, sizeof(struct node *),
					numroles);

	for (i = 0; i < numroles; i++) {
		struct node *role;
		guint32 position = node->_rolesposition + (i + 1) * sizeof(guint32);
		role = get_node(database, GET_VALUE(database->addr, position, guint32));
		g_array_append_val(node->roles, role);
	}
}

/* Find actor in index using binary search */
struct node *
find_actor(struct database *database, const gchar *name)
{
	guint32 begin = 0;
	guint32 end = database->numactors;

	while (begin != end) {
		guint middle = (end + begin) / 2;
		guint32 position = database->actors[middle];
		struct node *node;
		int cmp;

		node = get_node(database, position);
		cmp = strcmp(name, node->name);
		if (cmp < 0)
			end = middle;
		else if (cmp > 0)
			begin = middle + 1;
		else
			return node;
	}

	return NULL;
}
