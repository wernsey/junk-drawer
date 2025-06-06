/**
 *    **Random Dungeon Generator**
 *
 * A random dungeon generator for a Roguelike game.
 *
 * It is based on the "Rogue" algorithm, as described by Mark Damon Hughes
 * [Game Design: Article 07: Roguelike Dungeon Generation][algorithm], but with
 * some modifications.
 *
 * The basic algorithm is this:
 *
 * 1. Divide the map into a grid of `GRID_W` &times; `GRID_H` cells. Each cell will contain a room. The cells are
 *    marked as un_d_connected initially.
 * 2. Choose a random cell as the starting room; set it as the _current room_.
 * 3. While the _current room_ has un_d_connected neighbour cells,
 *    * mark the _current room_ as _d_connected,
 *    * set its _number_ to the number of rooms added so far,
 *    * mark it as a _path_ room, and
 *    * choose one of those un_d_connected neighbours at random and _d_connect it to the _current room_,
 *    * set the new room to the _current room_.
 * 4. Iteration stops when the current room reached a dead end.
 *    * The last room you added is labelled the end room.
 *    * You should now have a fairly long path from the start room to the end room.
 *    * The number of rooms added up to now is referred to as the _distance_:
 *      * If the distance is too short, it might be too easy for the adventurer to get from the start
 *        to the end, so the implementation has the option to go back to step 2
 * 5. Color the rooms: All the __d_connected_ rooms whose _number_ is less than $\frac{distance}{2}$ is
 *    colored blue, all the other rooms colored red.
 *    * Mark the _d_connection between the last blue room and the first red room as a door.
 * 6. While there are un_d_connected rooms on the grid:
 *    * choose a random un_d_connected room and _d_connect it to one of its _d_connected neighbours.
 *    * (If a room doesn't have _d_connected neighbours you can just retry with a different room;
 *      you'll _d_connect all of them eventually)
 *    * Mark the room as _d_connected. Give it the same _color_ as the neighbour you're _d_connecting it too.
 *    * The room is also marked as an _extra_ room.
 * 7. Add some random _d_connections between the rooms
 *    * Choose a room at random and _d_connect it to a random neighbour if they don't have a _d_connection yet.
 *    * If the two rooms have different colors, mark the _d_connection between them as a door...
 *    * ...unless the rooms are start/end rooms - we don't want a door directly to one of these rooms because
 *      it might make the map a bit too easy to solve.
 * 8. Some of the rooms are marked as _gone_ rooms - they will consist of just corridors, for effect.
 *    * The start room and end room cannot be gone rooms.
 *    * Rooms with only one exit can't be gone rooms either (they would just be corridors leading nowhere).
 * 9. Each room is given a random position and width/height to occupy within its cell on the grid.
 *
 * So when the algorithm is done, you can place the player in the room marked as the start room and place a key
 * in any of the blue rooms and you'll be guaranteed that the player will be able to find a key to open the
 * door in one of the accessible rooms. You'll also be guaranteed that the player will have to pass through
 * a door to reach the end room that will be in a room colored red.
 *
 * Of course, the _door_ and the _key_ doesn't have to be an actual door and key. The _door_ could be any obstacle
 * blocking the hero, like a river of lava, and the _key_ would be some means to overcome that obstacle, like a
 * lever to lower a d_drawbridge over the lava.
 *
 * All rooms marked as _path_ rooms will be guaranteed to lie on a path from the start room to the end room,
 * although other paths may exist thanks to the random _d_connections.
 *
 * The rooms marked as _extra_ rooms may be used for some additional purposes. For example, if an extra room has
 * only one corridor _d_connecting it to the rest of the maze, then it could be turned into a secret room by hiding
 * said corridor.
 *
 * You could put tougher _boss_ monsters and/or better treasure in rooms colored 1, so that the flow is that the
 * player starts in the blue area, fights a couple of easier enemies, find the key, go through the door to
 * the red area, encounter a boss, loot some treasure and reach the exit.
 *
 * * https://dysonlogos.blog/2013/12/23/the-key-to-all-this-madness/
 * * https://watabou.itch.io/one-page-dungeon
 * * https://www.patreon.com/posts/hatching-in-1pdg-31716880
 * * This online tool [svg-path-editor](https://yqnn.github.io/svg-path-editor/) helped me create the SVG paths.
 *
 * [algorithm]: https://web.archive.org/web/20131025132021/http://kuoi.org/~kamikaze/GameDesign/art07_rogue_dungeon.php
 *
 * Author: Werner Stoop
 * CC0 This work has been marked as dedicated to the public domain.
 * https://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef DUNGEON_H
#define DUNGEON_H

#define D_GRID_W 			5
#define D_GRID_H 			3

#define D_MAX_ROOM_SIZE		4
#define D_MIN_ROOM_SIZE		3

#define D_MIN_DISTANCE		7

#define D_SECRET_PROB		50

#define D_MAX_GONE_ROOMS 	2
#define D_MIN_GONE_ROOMS	1

#define D_COLOR_BLUE		0
#define D_COLOR_RED			1

#define D_FLAG_START_ROOM	0x01
#define D_FLAG_END_ROOM		0x02
#define D_FLAG_PATH_ROOM	0x04
#define D_FLAG_XTRA_ROOM	0x08
#define D_FLAG_GONE_ROOM	0x10
#define D_FLAG_SECRET_ROOM	0x20

#define D_NORTH				0x01
#define D_SOUTH				0x02
#define D_EAST				0x04
#define D_WEST				0x08

#define D_ORIENT_EW			(D_EAST | D_WEST)
#define D_ORIENT_NS			(D_NORTH | D_SOUTH)

#define D_TILE_EMPTY		' '
#define D_TILE_WALL			'#'
#define D_TILE_FLOOR		'.'
#define D_TILE_FLOOR_EDGE	','
#define D_TILE_CORRIDOR		'`'
#define D_TILE_CROSSING		'+'
#define D_TILE_ENTRANCE		'@'
#define D_TILE_EXIT			'$'
#define D_TILE_LOCKED_DOOR	'D'
#define D_TILE_THRESHOLD	't'
#define D_TILE_SECRET		's'

/* Choose a random number in [0,N) */
#ifndef D_RAND
#  define D_RAND(N)  ((rand() >> 8) % (N))
#endif

typedef struct {
	short x, y;
} D_Point;

typedef struct {
	char _d_connected;
	char color;
	unsigned char flags;
	char visited;

	short x, y, w, h;
} D_Room;

/* Connections between the rooms. An edge p,q means that
there's a corridor between rooms[p] and rooms[q] */
typedef struct {
	short p, q;
	char door, direction, offset;
	short index;

	short x, y, w, h;
} D_Edge;

typedef struct {

	/* These fields are used for configuration */

	short grid_w, grid_h;

	short min_room_size, max_room_size;

	short min_distance;

	short secret_prob;

	short min_gone_rooms, max_gone_rooms;

	/* These fields are computed; don't mess with them: */

	short map_w, map_h;

	D_Room *rooms;
	short n_rooms;

	char *map;

	D_Edge *edges;
	short n_edges, max_edges;

	short start_room, end_room;

	/* Temporary indexes */
	short *cells;

} D_Dungeon;


void d_init(D_Dungeon *M);

void d_deinit(D_Dungeon *M);

int d_generate(D_Dungeon *D);

int d_is_wall(D_Dungeon *M, int x, int y);

int d_is_floor(D_Dungeon *M, int x, int y);

void d_set_tile(D_Dungeon *M, int x, int y, char tile);

char d_get_tile(D_Dungeon *M, int x, int y);

D_Room *d_room_at(D_Dungeon *M, int x, int y);
D_Edge *d_corridor_at(D_Dungeon *M, int x, int y);

D_Room *d_random_room(D_Dungeon *M);

int d_random_wall(D_Dungeon *M, D_Room *room, D_Point *p);

#if defined(EOF)
/* include <stdio.h> first if you want to use this: */
void d_draw(D_Dungeon *M, FILE *f);
#endif

#ifdef DUNGEON_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void d_init(D_Dungeon *M) {
	M->grid_w = D_GRID_W;
	M->grid_h = D_GRID_H;

	M->min_room_size = D_MIN_ROOM_SIZE;
	M->max_room_size = D_MAX_ROOM_SIZE;

	M->min_distance = D_MIN_DISTANCE;

	M->secret_prob = D_SECRET_PROB;

	M->min_gone_rooms = D_MIN_GONE_ROOMS;
	M->max_gone_rooms = D_MAX_GONE_ROOMS;

	M->rooms = NULL;
	M->map = NULL;
	M->edges = NULL;
	M->cells = NULL;
}

void d_deinit(D_Dungeon *M) {
	if(M->rooms)
		free(M->rooms);
	M->rooms = NULL;

	if(M->map)
		free(M->map);
	M->map = NULL;

	if(M->edges)
		free(M->edges);
	M->edges = NULL;

	if(M->cells)
		free(M->cells);
	M->cells = NULL;
}

static void _d_reset_generator(D_Dungeon *M) {
	int i;

	memset(M->rooms, 0, M->n_rooms * sizeof *M->rooms);

	for(i = 0; i < M->map_w * M->map_h; i++) {
		M->map[i] = ' ';
	}

	M->n_edges = 0;
	memset(M->edges, 0, M->max_edges * sizeof *M->edges);
}

static int _d_init_generator(D_Dungeon *M) {

	M->map_w = M->grid_w * (M->max_room_size + 1) + 1;
	M->map_h = M->grid_h * (M->max_room_size + 1) + 1;

	M->n_rooms = M->grid_w * M->grid_h;

	M->rooms = malloc(M->n_rooms * sizeof *M->rooms);
	if(!M->rooms) {
		return 0;
	}

	M->map = malloc(M->map_w * M->map_h * sizeof *M->map);
	if(!M->map) {
		free(M->rooms);
		return 0;
	}

	/* Maximum number of edges in a M*N rooms graph is 2MN-M-N */
	M->max_edges = (2 * M->grid_w * M->grid_h) - M->grid_w - M->grid_h;
	M->edges = malloc(M->max_edges * sizeof *M->edges);
	if(!M->edges) {
		free(M->rooms);
		free(M->map);
		return 0;
	}

	return 1;
}

static int _d_grid_index(D_Dungeon *M, int row, int col) {
	if(row < 0 || row >= M->grid_h) return -1;
	if(col < 0 || col >= M->grid_w) return -1;
	return row * M->grid_w + col;
}

static int _d_get_unconnected(D_Dungeon *M) {
	int i, n = 0;
	assert(M->cells);
	for(i = 0; i < M->n_rooms; i++) {
		if(!M->rooms[i]._d_connected) {
			M->cells[n++] = i;
		}
	}
	return n;
}

static int _d_get_unconnected_neighbors(D_Dungeon *M, int gi, int n[4]) {
	int j = 0, i, count;
	int row = gi / M->grid_w;
	int col = gi % M->grid_w;

	i = _d_grid_index(M, row-1, col);
	if(i >= 0 && !M->rooms[i]._d_connected)
		n[j++] = i;
	i = _d_grid_index(M, row+1, col);
	if(i >= 0 && !M->rooms[i]._d_connected)
		n[j++] = i;
	i = _d_grid_index(M, row, col-1);
	if(i >= 0 && !M->rooms[i]._d_connected)
		n[j++] = i;
	i = _d_grid_index(M, row, col+1);
	if(i >= 0 && !M->rooms[i]._d_connected)
		n[j++] = i;

	count = j;
	while(j < 4) n[j++] = -1;
	return count;
}

static int _d_get_connected_neighbors(D_Dungeon *M, int gi, int n[4]) {
	int j = 0, i, count;
	int row = gi / M->grid_w;
	int col = gi % M->grid_w;

	i = _d_grid_index(M, row-1, col);
	if(i >= 0 && M->rooms[i]._d_connected)
		n[j++] = i;
	i = _d_grid_index(M, row+1, col);
	if(i >= 0 && M->rooms[i]._d_connected)
		n[j++] = i;
	i = _d_grid_index(M, row, col-1);
	if(i >= 0 && M->rooms[i]._d_connected)
		n[j++] = i;
	i = _d_grid_index(M, row, col+1);
	if(i >= 0 && M->rooms[i]._d_connected)
		n[j++] = i;

	count = j;
	while(j < 4) n[j++] = -1;
	return count;
}

static D_Edge *_d_get_edge(D_Dungeon *M, int p, int q) {
	int i;
	for(i = 0; i < M->n_edges; i++) {
		D_Edge *e = &M->edges[i];
		if((e->p == p && e->q == q) || (e->p == q && e->q == p))
			return e;
	}
	return NULL;
}

static D_Edge *_d_connect(D_Dungeon *M, int from, int to) {
	D_Edge *e;
	assert(!_d_get_edge(M, from, to));
	assert(M->n_edges < M->max_edges);
	e = &M->edges[M->n_edges];

	e->index = M->n_edges;
	e->p = from;
	e->q = to;

	M->n_edges++;
	return e;
}

static int _d_count_exits(D_Dungeon *M, int room) {
	int i, c = 0;
	for(i = 0; i < M->n_edges; i++) {
		if(M->edges[i].p == room || M->edges[i].q == room)
			c++;
	}
	return c;
}

static int _d_layout(D_Dungeon *M) {
	int count;
	int neighbours[4];

	int curr, next, split, c, attempts = 0;
	D_Edge *edge;

	if(!_d_init_generator(M))
		return 0;

	M->cells = calloc(M->n_rooms, sizeof *M->cells);
	if(!M->cells)
		return 0;

	do {
		_d_reset_generator(M);

		count = 0;

		curr = D_RAND(M->n_rooms);
		M->start_room = curr;
		for(;;) {
			M->cells[count++] = curr;

			M->rooms[curr]._d_connected = 1;
			M->rooms[curr].flags |= D_FLAG_PATH_ROOM;
			c = _d_get_unconnected_neighbors(M, curr, neighbours);
			if(!c) {
				M->end_room = curr;
				break;
			}

			next = neighbours[D_RAND(c)];

			_d_connect(M, curr, next);
			curr = next;
		}
	} while (count < M->min_distance  && ++attempts < 10);

	/* Color the first half of the rooms BLUE and the
	second half of the rooms RED. We put a locked door
	between the BLUE and RED rooms. */
	split = count / 2;
	curr = M->cells[split];
	next = M->cells[split+1];
	assert(curr >= 0 && next >= 0);
	edge = _d_get_edge(M, curr, next);
	assert(edge != NULL);
	edge->door = 1;

	for(c = 0; c < count; c++) {
		int i = M->cells[c];
		D_Room *r = &M->rooms[i];
		if(c <= split)
			r->color = D_COLOR_BLUE;
		else
			r->color = D_COLOR_RED;
	}

#if 1
	/* Connect all the un_d_connected rooms... */
	attempts = 0;
	for(;;) {
		c = _d_get_unconnected(M);
		if(!c) break;
		curr = M->cells[ D_RAND(c) ];

		c = _d_get_connected_neighbors(M, curr, neighbours);
		if(!c) continue;
		next = neighbours[ D_RAND(c) ];

		_d_connect(M, curr, next);
		M->rooms[curr]._d_connected = 1;

		/* Give it the same color */
		M->rooms[curr].color = M->rooms[next].color;

		/* Mark it as an "Extra D_Room". You might want to
		use the knowledge to use the knowledge later to
		hide a treasure there, or make it a secret room */
		M->rooms[curr].flags |= D_FLAG_XTRA_ROOM;
	}
#endif

#if 1
	/* Count the number of blue rooms */
	for(curr = 0, c = 0; curr < M->n_rooms; curr++) {
		if(M->rooms[curr].color == D_COLOR_BLUE)
			c++;
	}
	/* If there are more Red rooms (`n_rooms - c`) than blue rooms (`c`),
	swap everything around */
	if(M->n_rooms - c > c) {
		int t = M->start_room;
		M->start_room = M->end_room;
		M->end_room = t;
		for(curr = 0, c = 0; curr < M->n_rooms; curr++) {
			if(M->rooms[curr].color == D_COLOR_BLUE)
				M->rooms[curr].color = D_COLOR_RED;
			else
				M->rooms[curr].color = D_COLOR_BLUE;
		}
	}
#endif

	M->rooms[M->start_room].flags |= D_FLAG_START_ROOM;
	M->rooms[M->end_room].flags |= D_FLAG_END_ROOM;

#if 1
	count = D_RAND((M->grid_w + M->grid_h + 1)/2) + 1;
	while(count > 0) {
		int row, col, door;
		curr = D_RAND(M->n_rooms);
		row = curr / M->grid_w;
		col = curr % M->grid_w;

		if(++attempts > M->n_rooms) {
			/* Too many attempts - bail out
			Since I made it possible to add a door
			between rooms with different colors, I
			don't think this would be a problem */
			break;
		}

		/* Find candidate neighbours */
		c = 0;
		next = _d_grid_index(M, row-1, col);
		if(next >= 0 && !_d_get_edge(M, curr, next))
			neighbours[c++] = next;
		next = _d_grid_index(M, row+1, col);
		if(next >= 0 && !_d_get_edge(M, curr, next))
			neighbours[c++] = next;
		next = _d_grid_index(M, row, col-1);
		if(next >= 0 && !_d_get_edge(M, curr, next))
			neighbours[c++] = next;
		next = _d_grid_index(M, row, col+1);
		if(next >= 0 && !_d_get_edge(M, curr, next))
			neighbours[c++] = next;
		if(c == 0) {
			continue;
		}

		next = neighbours[ D_RAND(c) ];

		/* If the two rooms have different colors, put a locked door between them */
		door = M->rooms[curr].color != M->rooms[next].color;
		if(door) {
			if(M->rooms[curr].flags & (D_FLAG_START_ROOM | D_FLAG_END_ROOM)
				|| M->rooms[next].flags & (D_FLAG_START_ROOM | D_FLAG_END_ROOM)) {
				/* don't put the door in the start/end room - it might make the
				map too easy (as I found through experimentation) */
				continue;
			}
		}

		edge = _d_connect(M, curr, next);
		edge->door = door;
		count--;
	}
#endif

#if 1
	/* Make gone rooms */
	count = D_RAND(M->max_gone_rooms - M->min_gone_rooms + 1) + M->min_gone_rooms;
	attempts = 0;
	while(count > 0) {
		if(++attempts > 10) break;

		curr = D_RAND(M->n_rooms);

		if(_d_count_exits(M, curr) < 2)
			continue;
		if(M->rooms[curr].flags & (D_FLAG_GONE_ROOM | D_FLAG_START_ROOM | D_FLAG_END_ROOM))
			continue;

		M->rooms[curr].flags |= D_FLAG_GONE_ROOM;

		count--;
	}
#endif

	for(curr = 0; curr < M->n_rooms; curr++) {
		D_Room *room = &M->rooms[curr];

		int row = curr / M->grid_w;
		int col = curr % M->grid_w;

		if(M->rooms[curr].flags & D_FLAG_GONE_ROOM) {

			room->w = 1;
			room->h = 1;
			room->x = col * (M->max_room_size + 1) + 1 + M->max_room_size/2;
			room->y = row * (M->max_room_size + 1) + 1 + M->max_room_size/2;

		} else {
			room->w = D_RAND(M->max_room_size - M->min_room_size + 1) + M->min_room_size;
			room->h = D_RAND(M->max_room_size - M->min_room_size + 1) + M->min_room_size;

			room->x = col * (M->max_room_size + 1) + 1 + (M->max_room_size - room->w + 1)/2;
			room->y = row * (M->max_room_size + 1) + 1 + (M->max_room_size - room->h + 1)/2;
		}
	}

	/* Shift the edges up/down or left/right a bit */
	for(curr = 0; curr < M->n_edges; curr++) {
		M->edges[curr].offset = D_RAND(3)-1;
	}

	free(M->cells);
	M->cells = NULL;

	return 1;
}

int d_is_wall(D_Dungeon *M, int x, int y) {
	char t;
	if(x < 0 || x >= M->map_w || y < 0 || y >= M->map_h)
		return 1;
	t = M->map[ y * M->map_w + x ];
	return t == D_TILE_EMPTY || t == D_TILE_WALL;
}

int d_is_floor(D_Dungeon *M, int x, int y) {
	char t;
	if(x < 0 || x >= M->map_w || y < 0 || y >= M->map_h)
		return 0;
	t = M->map[ y * M->map_w + x ];
	return t == D_TILE_FLOOR_EDGE || t == D_TILE_FLOOR;
}

static int _d_is_tile(D_Dungeon *M, int x, int y, char tile) {
	if(x < 0 || x >= M->map_w || y < 0 || y >= M->map_h)
		return 0;
	return (M->map[ y * M->map_w + x ] == tile);
}

void d_set_tile(D_Dungeon *M, int x, int y, char tile) {
	if(x < 0 || x >= M->map_w || y < 0 || y >= M->map_h)
		return;
	M->map[ y * M->map_w + x ] = tile;
}

char d_get_tile(D_Dungeon *M, int x, int y) {
	if(x < 0 || x >= M->map_w || y < 0 || y >= M->map_h)
		return D_TILE_EMPTY;
	return M->map[ y * M->map_w + x ];
}

static int _d_has_neighbour(D_Dungeon *M, int x, int y, char tile) {
	return (_d_is_tile(M, x - 1, y, tile) ||
			_d_is_tile(M, x + 1, y, tile) ||
			_d_is_tile(M, x, y - 1, tile) ||
			_d_is_tile(M, x, y + 1, tile));
}

int d_random_wall(D_Dungeon *M, D_Room *room, D_Point *p) {
	int attempts = 0, x, y;
	do {
		switch(D_RAND(4)) {
		case 0:
			x = room->x - 1;
			y = room->y + D_RAND(room->h);
			if(d_is_floor(M, x + 1, y) && d_is_wall(M, x - 1, y) && d_is_wall(M, x, y - 1) && d_is_wall(M, x, y + 1)) {
				p->x = x; p->y = y;
				return 1;
			}
			break;
		case 1:
			x = room->x + room->w;
			y = room->y + D_RAND(room->h);
			if(d_is_floor(M, x - 1, y) && d_is_wall(M, x + 1, y) && d_is_wall(M, x, y - 1) && d_is_wall(M, x, y + 1)) {
				p->x = x; p->y = y;
				return 1;
			}
			break;
		case 2:
			x = room->x + D_RAND(room->w);
			y = room->y - 1;
			if(d_is_floor(M, x, y + 1) && d_is_wall(M, x, y - 1) && d_is_wall(M, x - 1, y) && d_is_wall(M, x + 1, y)) {
				p->x = x; p->y = y;
				return 1;
			}
			break;
		case 3:
			x = room->x + D_RAND(room->w);
			y = room->y + room->h;
			if(d_is_floor(M, x, y - 1) && d_is_wall(M, x, y + 1) && d_is_wall(M, x - 1, y) && d_is_wall(M, x + 1, y)) {
				p->x = x; p->y = y;
				return 1;
			}
			break;
		}
	} while(++attempts < 8);
	return 0;
}

static int _d_edge_direction(int p, int q) {
	if(q < p) {
		if(q == p - 1) {
			return D_WEST;
		} else {
			return D_NORTH;
		}
	} else {
		if(q == p + 1) {
			return D_EAST;
		} else {
			return D_SOUTH;
		}
	}
}

static const char *_d_dir_name(int dir) {
	switch(dir) {
		case D_NORTH: return "North";
		case D_SOUTH: return "South";
		case D_EAST: return "East";
		case D_WEST: return "West";
	}
	return "";
}

static int _d_traverse_room(D_Dungeon *M, int idx, int prev) {
	int i, e = 0, exits[4];
	D_Edge *edges[4];
	D_Point exit_points[4];
	D_Room *room = &M->rooms[idx];

	if(room->visited) return 0;
	room->visited = 1;
	printf("Room %d:\n", idx);

	for(i = 0; i < M->n_edges; i++) {
		if(M->edges[i].p == idx) {
			assert(e < 4);
			exits[e] = M->edges[i].q;
			edges[e] = &M->edges[i];
			e++;
		} else if(M->edges[i].q == idx) {
			assert(e < 4);
			exits[e] = M->edges[i].p;
			edges[e] = &M->edges[i];
			e++;
		}
	}

	for(i = 0; i < e; i++) {
		int dir = _d_edge_direction(idx, exits[i]);
		printf(" * %s %d %c\n", _d_dir_name(dir), exits[i], exits[i]==prev?'*':' ');
		switch(dir) {
			case D_NORTH:
				exit_points[i].x = edges[i]->x;
				exit_points[i].y = edges[i]->y + edges[i]->h - 1;
				break;
			case D_SOUTH:
				exit_points[i].x = edges[i]->x;
				exit_points[i].y = edges[i]->y;
				break;
			case D_WEST:
				exit_points[i].x = edges[i]->x + edges[i]->w - 1;
				exit_points[i].y = edges[i]->y;
				break;
			case D_EAST:
				exit_points[i].x = edges[i]->x;
				exit_points[i].y = edges[i]->y;
				break;
		}
	}

	for(i = 0; i < e; i++) {
		int s;
		if(exits[i] == prev) continue;
		s = _d_traverse_room(M, exits[i], idx);
		if(s == 1 && !(room->flags & D_FLAG_GONE_ROOM) && (M->rooms[ exits[i] ].flags & D_FLAG_XTRA_ROOM)) {
			if(D_RAND(100) > M->secret_prob) continue;
			M->rooms[ exits[i] ].flags |= D_FLAG_SECRET_ROOM;
			d_set_tile(M, exit_points[i].x, exit_points[i].y, D_TILE_SECRET);
		}
	}

	return e;
}

static void _d_postprocess(D_Dungeon *M) {
	int i, j, start, placed;
	D_Room *room;
	D_Edge *edge;
	D_Point point;

	for(j = 0; j < M->grid_h; j++) {
		for(i = 0; i < M->grid_w; i++) {
			D_Room *room = &M->rooms[_d_grid_index(M, j, i)];
			int p, q;

			if(!room->_d_connected || (room->flags & D_FLAG_GONE_ROOM)) {
				d_set_tile(M, room->x, room->y, D_TILE_CROSSING);
				continue;
			}

			for(p = 0; p < room->w; p++) {
				for(q = 0; q < room->h; q++) {
					d_set_tile(M, room->x + p, room->y + q, D_TILE_FLOOR);
				}
			}
		}
	}

	/* Draw all the edges, computing their bounding boxes in the process */
	for(i = 1; i < M->n_rooms; i++) {
		int row = i / M->grid_w;
		int col = i % M->grid_w;

		int x = col * (M->max_room_size + 1) + 1 + (M->max_room_size)/2;
		int y = row * (M->max_room_size + 1) + 1 + (M->max_room_size)/2;

		if(col > 0) {
			int left = i - 1;
			int maxx, minx, tx = x;

			edge = _d_get_edge(M, i, left);
			if(edge) {
				edge->direction = D_ORIENT_EW;
				if((M->rooms[i].flags & D_FLAG_GONE_ROOM) || (M->rooms[left].flags & D_FLAG_GONE_ROOM))
					edge->y = y;
				else
					edge->y = y + edge->offset;

				start = edge->y * M->map_w + x;

				edge->h = 1;
				maxx = x - (M->max_room_size + 1);
				minx = x;

				for(j = 0; j <= M->max_room_size + 1; j++, tx--) {
					if(M->map[ start - j ] != D_TILE_EMPTY)
						continue;

					if(tx < minx)
						minx = tx;
					if(tx > maxx)
						maxx = tx;

					if(edge->door && j == M->max_room_size/2 + 1)
						M->map[ start - j ] = D_TILE_LOCKED_DOOR;
					else
						M->map[ start - j ] = D_TILE_CORRIDOR;
				}

				edge->x = minx;
				edge->w = maxx - minx + 1;
			}
		}
		if(row > 0) {
			int up = i - M->grid_w;
			int maxy, miny, ty = y;

			edge = _d_get_edge(M, i, up);
			if(edge) {
				edge->direction = D_ORIENT_NS;
				if((M->rooms[i].flags & D_FLAG_GONE_ROOM) || (M->rooms[up].flags & D_FLAG_GONE_ROOM))
					edge->x = x;
				else
					edge->x = x + edge->offset;

				start = y * M->map_w + edge->x;

				edge->w = 1;
				maxy = y - (M->max_room_size + 1);
				miny = y;

				for(j = 0; j <= M->max_room_size; j++, ty--) {
					if(M->map[start - j * M->map_w] != D_TILE_EMPTY)
						continue;

					if(ty < miny)
						miny = ty;
					if(ty > maxy)
						maxy = ty;

					if(edge->door && j == M->max_room_size/2 + 1)
						M->map[ start - j * M->map_w ] = D_TILE_LOCKED_DOOR;
					else
						M->map[ start - j * M->map_w ] = D_TILE_CORRIDOR;
				}

				edge->y = miny;
				edge->h = maxy - miny + 1;
			}
		}
	}

	for(j = 0; j < M->map_h; j++) {
		for(i = 0; i < M->map_w; i++) {
			if(d_is_wall(M, i, j) && (
				!d_is_wall(M, i - 1, j) ||
				!d_is_wall(M, i + 1, j) ||
				!d_is_wall(M, i, j - 1) ||
				!d_is_wall(M, i, j + 1) ||
				/* Diagonal checks only makes it look nice in ASCII: */
				!d_is_wall(M, i - 1, j - 1) ||
				!d_is_wall(M, i + 1, j - 1) ||
				!d_is_wall(M, i - 1, j + 1) ||
				!d_is_wall(M, i + 1, j + 1)
			)) {
				d_set_tile(M, i, j, D_TILE_WALL);
			}

			if(_d_is_tile(M, i, j, D_TILE_CORRIDOR) && _d_has_neighbour(M, i, j, D_TILE_FLOOR)) {
				D_Edge * e = d_corridor_at(M, i, j);
				if(!e->door)
					d_set_tile(M, i, j, D_TILE_THRESHOLD);
			}
		}
	}

	/* Place the dungeon entrance and exit */
	room = &M->rooms[M->start_room];
	placed = d_random_wall(M, room, &point);
	if(placed) {
		d_set_tile(M, point.x, point.y, D_TILE_ENTRANCE);
	} else {
		/* Ugly, but better than nothing */
		d_set_tile(M, room->x + room->w/2, room->y + room->h/2, D_TILE_ENTRANCE);
	}

	room = &M->rooms[M->end_room];
	placed = d_random_wall(M, room, &point);
	if(placed) {
		d_set_tile(M, point.x, point.y, D_TILE_EXIT);
	} else {
		d_set_tile(M, room->x + room->w/2, room->y + room->h/2, D_TILE_EXIT);
	}

	for(j = 0; j < M->map_h; j++) {
		for(i = 0; i < M->map_w; i++) {
			if(_d_is_tile(M, i, j, D_TILE_FLOOR) && _d_has_neighbour(M, i, j, D_TILE_WALL) &&
			!(_d_has_neighbour(M, i, j, D_TILE_THRESHOLD)
				|| _d_has_neighbour(M, i, j, D_TILE_CORRIDOR)
				|| _d_has_neighbour(M, i, j, D_TILE_LOCKED_DOOR)
				|| _d_has_neighbour(M, i, j, D_TILE_ENTRANCE)
				|| _d_has_neighbour(M, i, j, D_TILE_EXIT)
				|| _d_has_neighbour(M, i, j, D_TILE_SECRET)))
				M->map[ j * M->map_w + i ] = D_TILE_FLOOR_EDGE;
		}
	}

	_d_traverse_room(M, M->start_room, -1);
}

int d_generate(D_Dungeon *D) {
	if(!_d_layout(D))
		return 0;
	_d_postprocess(D);
	return 1;
}

D_Room *d_room_at(D_Dungeon *M, int x, int y) {
	int col = (x - 1) / (M->max_room_size + 1);
	int row = (y - 1) / (M->max_room_size + 1);
	int i = _d_grid_index(M, row, col);
	D_Room *r = &M->rooms[i];
	if(x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h)
		return r;
	return NULL;
}

D_Edge *d_corridor_at(D_Dungeon *M, int x, int y) {
	int i;
	for(i = 0; i < M->n_edges; i++) {
		D_Edge *e = &M->edges[i];
		if(x >= e->x && x < e->x + e->w && y >= e->y && y < e->y + e->h)
			return e;
	}
	return NULL;
}

D_Room *d_random_room(D_Dungeon *M) {
	D_Room *r;
	do {
		r = &M->rooms[D_RAND(M->n_rooms)];
	} while(r->flags & D_FLAG_GONE_ROOM);
	return r;
}

void d_draw(D_Dungeon *M, FILE *f) {
	int i, j;
	for(j = 0; j < M->map_h; j++) {
		for(i = 0; i < M->map_w; i++) {
			putc(d_get_tile(M, i, j), f);
		}
		putc('\n', f);
	}
}

#endif /* DUNGEON_IMPLEMENTATION */
#endif /* DUNGEON_H */

