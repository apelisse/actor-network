#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "pack.h"

guint
compute_distance(struct node *to)
{
	guint distance = 0;

	while (to->from) {
		if (to->from->type != kPerson)
			distance++;
		to = to->from;
	}

	return distance;
}

guint
find_shortest_path(struct database *database,
		   struct node *from, struct node *to,
		   guint8 typemask)
{
	GQueue queue = G_QUEUE_INIT;

	g_queue_push_tail(&queue, from);

	while (!g_queue_is_empty(&queue)) {
		struct node *node = g_queue_pop_head(&queue);
		guint i;

		get_roles(database, node);
		for (i = 0; i < node->roles->len; i++) {
			struct node *movie = ((struct node **)node->roles->data)[i];
			guint j;

			if (!(1 << movie->type & typemask))
				continue;

			/* not visited */
			if (movie->from != NULL)
				continue;

			movie->from = node;

			get_roles(database, movie);
			for (j = 0; j < movie->roles->len; j++) {
				struct node *actor = ((struct node **)movie->roles->data)[j];

				/* not visited */
				if (actor->from != NULL || actor == from)
					continue;

				actor->from = movie;

				if (actor == to)
					return compute_distance(actor);

				g_queue_push_tail(&queue, actor);
			}
		}
	}

	return G_MAXUINT32;
}

char *
escape_dquotes(const gchar *name)
{
	size_t i;
	GString *string = g_string_new("");

	for (i = 0; i < strlen(name); i++) {
		if (name[i] == '\"')
			g_string_append(string, "\\\"");
		else
			g_string_append_c(string, name[i]);
	}

	return g_string_free(string, FALSE);
}

void
display_graph(struct node *from, struct node *to, guint distance)
{
	struct node *node;

	puts("graph \"Bacon Graph\" {");

	printf("label = \"Distance found: %u\"\n", distance);
	puts("labelloc = top");

	printf("\"%s\" [ color = red ]\n", from->name);
	printf("\"%s\" [ color = red ]\n", to->name);

	node = to;
	while (node->from) {
		struct node *from = node->from;
		char *fromname, *name;

		name = escape_dquotes(node->name);
		fromname = escape_dquotes(node->from->name);

		if (from->type != kPerson)
			printf("\"%s\" [ shape = record, label = \"%s (%u)\" ]\n",
			       fromname, fromname, node->from->year);
		else
			printf("\"%s\"\n", fromname);

		printf("\"%s\" -- \"%s\"\n", name, fromname);

		g_free(name);
		g_free(fromname);

		node = from;
	}

	puts("}");
}

int
main(int argc, char **argv)
{
	struct database *database;
	gboolean dot, series, movies, videos, games, tv;
	struct node *from, *to;
	guint distance;
	guint8 mask;

	dot = series = movies = videos = games = tv = 0;

	GOptionEntry entries[] = {
		{ "dot", 'd', 0, G_OPTION_ARG_NONE, &dot,
		  "Output as Dot file (graphviz)", NULL },
		{ "series", 's', 0, G_OPTION_ARG_NONE, &series,
		  "Exclude Series from computation", NULL },
		{ "movies", 'm', 0, G_OPTION_ARG_NONE, &movies,
		  "Exclude Movies from computation", NULL },
		{ "videos", 'v', 0, G_OPTION_ARG_NONE, &videos,
		  "Exclude Videos from computation", NULL },
		{ "games", 'g', 0, G_OPTION_ARG_NONE, &games,
		  "Exclude Video Games from computation", NULL },
		{ "tv", 't', 0, G_OPTION_ARG_NONE, &tv,
		  "Exclude TV shows from computation", NULL },
		{ NULL }
	};
	GOptionContext *context;
	GError *error = NULL;

	context = g_option_context_new (
		"database first_actor second_actor ... - Calculate bacon number");
	g_option_context_add_main_entries (context, entries, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_print ("option parsing failed: %s\n", error->message);
		return 1;
	}

	if (argc != 4) {
		fprintf(stderr, "%s",
			g_option_context_get_help(context, TRUE, NULL));
		return 1;
	}

	database = open_database(argv[1]);

	from = find_actor(database, argv[2]);
	if (from == NULL) {
		fprintf(stderr, "Can't find actor: %s\n", argv[2]);
		return 1;
	}

	to = find_actor(database, argv[3]);
	if (to == NULL) {
		fprintf(stderr, "Can't find actor: %s\n", argv[3]);
		return 1;
	}

	mask = 0xFF;
	if (series)
		mask &= ~(1 << kSeries);
	if (movies)
		mask &= ~(1 << kMovie);
	if (videos)
		mask &= ~(1 << kVideo);
	if (games)
		mask &= ~(1 << kGame);
	if (tv)
		mask &= ~(1 << kTV);

	distance = find_shortest_path(database, from, to, mask);
	if (distance == G_MAXUINT) {
		fprintf(stderr, "No path found\n");
		return 1;
	}

	display_graph(from, to, distance);

	return 0;
}
