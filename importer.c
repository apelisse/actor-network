#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#define MAX_LINE 512

gint
stringcmp(gconstpointer a, gconstpointer b, gpointer data)
{
	(void)data;

	return strcmp(a, b);
}

/* Node can be actor or movie */
/* Type is kPerson for actors */
struct node {
	const char *name;
	size_t length;
	GArray *roles;
	guint32 offset;
	enum type { kPerson, kMovie, kSeries, kTV, kVideo, kGame } type;
	guint32 year;
	char align;
};

gint
yearcmp(gconstpointer a, gconstpointer b)
{
	const struct node *one = *(struct node **)a;
	const struct node *other = *(struct node **)b;

	return (one->year - other->year);
}

void
free_node(gpointer data)
{
	struct node *node = data;

	g_array_free(node->roles, TRUE);
	g_free(node);
}

struct node *
new_node(const char *name, size_t length)
{
	struct node *node = g_new(struct node, 1);

	node->name = name;
	node->length = length;
	node->roles = g_array_new(FALSE, FALSE, sizeof(struct node *));
	node->type = kPerson;
	node->year = 0;

	return node;
}

#define ALIGN(off) ((off + 3) & ~3)

gboolean
traverse_setoffset(gpointer key, gpointer value, gpointer data)
{
	guint32 *offset = data;
	struct node *node = value;

	(void)key;

	node->offset = *offset;

	*offset += sizeof(guint32);
	*offset += node->length;

	/* This is room for "type" */
	*offset += sizeof(guint8);

	node->align = ALIGN(*offset) - *offset;
	*offset += node->align;

	if (node->type != kPerson)
		*offset += sizeof(node->year);

	/* Number of roles + Reference of each role */
	*offset += sizeof(node->roles->len);
	*offset += node->roles->len * sizeof(node->offset);

	return FALSE;
}

struct nodeid {
	int fd;
	guint32 id;
};

gboolean
traverse_writenode(gpointer key, gpointer value, gpointer data)
{
	struct node *node = value;
	struct nodeid *nodeid = data;
	int fd = nodeid->fd;
	guint32 full = ~0, empty = 0;
	guint i;

	(void)key;

	write(fd, &nodeid->id, sizeof(nodeid->id));
	nodeid->id++;
	write(fd, key, node->length - 1);
	write(fd, &empty, sizeof(char));

	write(fd, &node->type, sizeof(guint8));

	write(fd, &full, node->align);

	if (node->type != kPerson)
		write(fd, &node->year, sizeof(node->year));
	else
		g_array_sort(node->roles, yearcmp);

	write(fd, &node->roles->len, sizeof(node->roles->len));

	for (i = 0; i < node->roles->len; i++) {
		struct node *tmpnode = *((struct node **)node->roles->data + i);

		write(fd, &tmpnode->offset, sizeof(tmpnode->offset));
	}

	return FALSE;
}

gboolean
traverse_writeindex(gpointer key, gpointer value, gpointer data)
{
	struct node *node = value;
	int fd = *(int*)data;

	(void)key;

	write(fd, &node->offset, sizeof(node->offset));

	return FALSE;
}

void
write_nodes(int fd, GTree *actors, GHashTable *movies)
{
	struct nodeid nodeid = { fd, 0 };
	g_tree_foreach(actors, traverse_writenode, &nodeid);
	g_hash_table_find(movies, traverse_writenode, &nodeid);
}

int
main(int argc, char **argv)
{
	GTree *actors;
	GHashTable *movies;
	struct node *actor, *movie;
	char line[MAX_LINE];
	guint32 offset, numrelations = 0;
	int fd, num;

	if (argc < 2) {
		fprintf(stderr, "You must specify index file\n");
		return 1;
	}

	fd = open(argv[1], O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		perror("open");
		return 2;
	}

	actors = g_tree_new_full(stringcmp, NULL, g_free, free_node);
	movies = g_hash_table_new_full(g_str_hash, g_str_equal,
				       g_free, free_node);

	actor = movie = NULL;
	while (fgets(line, MAX_LINE, stdin)) {
		size_t length;

		length = strlen(line);
		line[length - 1] = '\0';
		if (!strncmp(line, "actor ", 6)) {
			char *actorname;
			if (!g_tree_lookup_extended(actors, line + 6,
						   (void**)&actorname,
						   (void**)&actor)) {
				actorname = g_strdup(line + 6);
				actor = new_node(actorname, length - 6);
				g_tree_insert(actors, actorname, actor);
			}
		} else if (!strncmp(line, "movie ", 6)) {
			char *moviename;

			g_assert(actor != NULL);

			numrelations++;

			if (!g_hash_table_lookup_extended(movies, line + 6,
							  (void**)&moviename,
							  (void**)&movie)) {
				moviename = g_strdup(line + 6);
				// Remove year from movie name
				movie = new_node(moviename, length - 11);
				movie->year = strtol(moviename + length - 11,
						     NULL, 10);
				g_hash_table_insert(movies, moviename, movie);
			}
			g_array_append_val(actor->roles, movie);
			g_array_append_val(movie->roles, actor);
		} else if (!strncmp(line, "type ", 5)) {
			char *type = line + 5;
			g_assert(movie != NULL);
			if (!strcmp(type, "movie"))
				movie->type = kMovie;
			else if (!strcmp(type, "series"))
				movie->type = kSeries;
			else if (!strcmp(type, "game"))
				movie->type = kGame;
			else if (!strcmp(type, "tv"))
				movie->type = kTV;
			else if (!strcmp(type, "video"))
				movie->type = kVideo;
		} else {
			fprintf(stderr, "Invalid command: %s\n", line);
		}
	}

	offset = sizeof(offset) + sizeof(num);
	g_tree_foreach(actors, traverse_setoffset, &offset);
	g_hash_table_find(movies, traverse_setoffset, &offset);
	write(fd, &offset, sizeof(offset));
	num = g_tree_nnodes(actors) + g_hash_table_size(movies);
	write(fd, &num, sizeof(num));

	write_nodes(fd, actors, movies);

	printf("Imported:\n"
	       "%u movies\n", g_hash_table_size(movies));
	/*
	 * We don't need indexes on movies
	 * Note that it will break cross-references from actors, so you
	 * should stop using it.
	 */
	g_hash_table_destroy(movies);

	num = g_tree_nnodes(actors);
	printf("%u actors\n"
	       "%u roles\n", num, numrelations);
	write(fd, &num, sizeof(num));
	g_tree_foreach(actors, traverse_writeindex, &fd);
	g_tree_destroy(actors);

	close(fd);

	return 0;
}
