# -*- coding: utf-8 -*-

def main():
    import argparse
    import random

    parser = argparse.ArgumentParser("Generate list of movies and actors")
    parser.add_argument("-a", "--actors", dest="actors", type=int,
                        help="Number of actors", default=100)
    parser.add_argument("-m", "--movies", dest="movies", type=int,
                        help="Number of movies (may not be smaller)",
                        default=100)
    parser.add_argument("-r", "--relations", dest="relations", type=int,
                        help="Average number of actors per movies",
                        default=10)
    parser.add_argument("-d", "--deviation", dest="deviation", type=int,
                        help="Standard deviation to the average relations",
                        default=5)

    args = parser.parse_args()
    for actor in range(args.actors):
        print 'actor Actor', actor
        num_movies = int(random.normalvariate(args.relations + 1,
                                              args.deviation))
        for movie in range(num_movies):
            print 'movie Movie', random.randrange(args.movies), 2000
            print 'type movie'

if __name__ == '__main__':
    main()
