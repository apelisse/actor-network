=============
Actor Network
=============

This can be used to calculate bacon number between two actors (it
doesn't have to be with Kevin Bacon), and to display the result as a
graph.

It requires several step to make it work, but some shortcuts can be taken:

- Create all the commands to describe the graph (that can be done with
  ``parse_actors.py`` or ``generator.py``)
- Convert the commands into an actual graph database (using ``importer`` C
  program)
- Read the database and display the resulting graph (using ``bacon`` C program)

0. Compiling the application
----------------------------

The first thing you will need to do it to compile the application:

    $ make

You can change some parameters, creating a ``config.mak`` with, for
example ``CFLAGS`` and ``CC`` variables.

Building the application will require you to have ``glib-2`` development
library.

1. Building the database
------------------------

Of course, you need to build a database to use the program. There are
two ways to create a database, either you generate a random one (that is
fast, nothing to download), or you do it from IMDb database files (but
you need to download it first).

a. The fast way
...............

The fastest way to build a graph database is to generate a random graph
using the ``generator.py``. It will build a random graph, where:

- Every actor will be called *Actor %i*, with **%i** being an integer
- Every movie will be called *Movie %i*, with **%i** also being an integer

You can choose the number of movies and actors, and also the average
number of movies per actors.

The output of ``generator.py`` should be passed to ``importer``

    $ python generate.py | ./importer graph.db

This will build a very small database (thus very fast to build). If you
have more time (probably a couple of minutes), you can build a
"real-life" database:

    $ python generate.py -a 2600000 -m 2000000 -r 8 -d 4 | ./importer graph.db

b. The real way
...............

If you agree with IMDb policy, you can download the following files from
http://www.imdb.com/interfaces :

- actors.list
- actresses.list

Then, run the ``parse_actors.py`` script:

    $ python parse_actors.py actors.list actresses.list | ./importer graph.db

2. Using the database
---------------------

The simplest way to use the database, is to use the ``run.sh`` script that
will generate the png file for you, and display it. There are a couple
of dependencies though:

- ``graphvi dot`` is required to interpet the dot graph. It should be
  compiled with png support
- ``ImageMagic display`` will be required to display the image. Of course
  you can replace it by the program of your choice.

Using the ``run.sh`` is pretty straigt-forward:

    $ ./run.sh graph.db "De Niro, Robert" "Bacon, Kevin (I)"

Or if you generated the graph using the fast method:

    $ ./run.sh graph.db "Actor 0" "Actor 1"

3. Known problems
-----------------

The biggest known problem is that you have to type the exact name of the
actor you are looking for. For example, if you want to look for *"Kevin
Bacon"*, you will have to use his full IMDb name, that is: *"Bacon,
Kevin (I)"*. There is no fuzzy matching, or partial matching in the name.
