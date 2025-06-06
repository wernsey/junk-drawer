/*
 * Generates a SVG of a dungeon generated in `dungeon.c` in the style of
 * [dysonlogos][] using the Poisson disc-based [hatching][] technique
 * that [watabou][] used in his dungeon generator.
 *
 * [watabou]: https://watabou.itch.io/one-page-dungeon
 * [hatching]: https://www.patreon.com/posts/hatching-in-1pdg-31716880
 * [dysonlogos]: https://dysonlogos.blog/2013/12/23/the-key-to-all-this-madness/
 *
 * Author: Werner Stoop
 * CC0 This work has been marked as dedicated to the public domain.
 * https://creativecommons.org/publicdomain/zero/1.0/
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define DUNGEON_IMPLEMENTATION
#include "dungeon.c"

#define POISSON_IMPLEMENTATION
#include "poisson.c"

static float frand() {
	return ((float)rand())/RAND_MAX;
}

static int wall_constraint(struct poisson *P, float x, float y) {
	D_Dungeon *M = P->data;
	float fx, fy;
	int mx, my;

	(void)P;

	x /= 10.0;
	y /= 10.0;

	mx = (int)x;
	my = (int)y;

	fx = x - mx;
	fy = y - my;

	if(!d_is_wall(M, mx, my)) return 1;

	if(fx < 0.3 && !d_is_wall(M, mx - 1, my)) return 1;
	if(fx > 0.7 && !d_is_wall(M, mx + 1, my)) return 1;
	if(fy < 0.3 && !d_is_wall(M, mx, my - 1)) return 1;
	if(fy > 0.7 && !d_is_wall(M, mx, my + 1)) return 1;

	return 0;
}

static void drawSvg(D_Dungeon *M, FILE *f) {
	int i, j;
	struct poisson P;
	D_Room *r;

	poisson_init(&P);
	P.r = 2.75;
	P.w = (M->map_w+1) * 10;
	P.h = (M->map_h+1) * 10;
	P.x0.y = (M->rooms[0].y + M->rooms[0].h/2) * 10;
	P.x0.x = (M->rooms[0].x + M->rooms[0].w/2) * 10;
	P.constraint = wall_constraint;
	P.data = M;

	if(!poisson_plot(&P)) {
		fprintf(stderr, "Couldn't do Poisson Disk thing :(\n");
		/* return; */
	}

	fprintf(f, "<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\">\n", M->map_w * 10, M->map_h * 10);

	fprintf(f, " <defs>\n");
	fprintf(f, " <symbol id=\"floor\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <rect x=\"-0.2\" y=\"-0.2\" width=\"10.4\" height=\"10.4\" fill=\"#FFF\"/>\n");
	fprintf(f, " </symbol>\n");

	fprintf(f, " <symbol id=\"wall-l\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 0 C 0.2 2.5, -0.2 7.5, 0 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"wall-r\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 10 0 C 10.2 2.5, 9.8 7.5, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"wall-t\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 0 C 2.5 0.2, 7.5 -0.2, 10 0\" stroke=\"black\" fill=\"none\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"wall-b\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 10 C 2.5 10.2, 7.5 9.8, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");

	fprintf(f, " <symbol id=\"floor-r1\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 10 0 C 10.2 2.5, 9.8 7.5, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.125\"  stroke-dasharray=\"0.3 1.5 1 2.5 0.4 2\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"floor-r2\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 10 0 C 10.2 2.5, 9.8 7.5, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.125\"  stroke-dasharray=\"0.7 2.5 0.7 1.5 0.5 2\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"floor-r3\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 10 0 C 10.2 2.5, 9.8 7.5, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.125\"  stroke-dasharray=\"0.7 1.5 0.7 1 0.5 2\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"floor-b1\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 10 C 2.5 10.2, 7.5 9.8, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.125\"  stroke-dasharray=\"0.3 1.5 1 2.5 0.4 2\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"floor-b2\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 10 C 2.5 10.2, 7.5 9.8, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.125\"  stroke-dasharray=\"0.7 2.5 0.7 1.5 0.5 2\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"floor-b3\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 10 C 2.5 10.2, 7.5 9.8, 10 10\" stroke=\"black\" fill=\"none\" stroke-width=\"0.125\"  stroke-dasharray=\"0.7 1.5 0.7 1 0.5 2\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");

	fprintf(f, " <symbol id=\"blobs\" width=\"6\" height=\"6\" viewBox=\"-3 -3 6 6\" x=\"-3\" y=\"-3\">\n");
	fprintf(f, "  <path d=\"M -0.01 -2.88 C -0.86 -2.92 -1.02 -2.89 -2.22 -2.13 C -2.58 -1.96 -2.83 -1.13 -2.89 -0.08 C -3 0.63 -2.61 1.18 -2.2 2.01 C -1.66 2.77 -0.59 2.73 -0.01 2.73 C 0.69 2.8 1.79 2.73 2.17 2.47 C 2.84 2.24 2.86 0.82 2.85 -0.05 C 2.83 -0.68 2.78 -2.05 2.29 -2.1 C 1.6 -2.31 1.21 -2.92 0 -2.89\"\n");
	fprintf(f, "  fill=\"#EEE\" stroke=\"none\"/>\n");
	fprintf(f, " </symbol>\n\n");
	fprintf(f, "\n");

	fprintf(f, " <symbol id=\"strokes\" width=\"4\" height=\"4\" viewBox=\"-2 -2 4 4\" x=\"-2\" y=\"-2\">\n");
	fprintf(f, "  <path d=\"M-1.5,-1.5 C -1.75,-0.5 -1.25,0.5 -1.5,1.5 M0,-1.75 C -0.2,-0.5 0.2,0.5 0,1.5   M1.5,-1.5 C 1.75,-0.5 1.25,0.5 1.5,1.5\"\n");
	fprintf(f, "  fill=\"none\" stroke=\"black\" stroke-width=\"0.25\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"stone1\" width=\"2\" height=\"2\" viewBox=\"-1 -1 2 2\" x=\"-1\" y=\"-1\">\n");
	fprintf(f, "  <path d=\"M -0.66 -0.7 Q -0.96 -0.45 -0.88 0.17 Q -0.9 0.4 -0.5 0.63 Q 0.065 0.87 0.44 0.76 C 0.9 0.56 1 -0.78 0.59 -0.8 C 0.15 -0.87 -0.54 -1.01 -0.66 -0.7 Z\" \n");
	fprintf(f, "   fill=\"white\" stroke=\"black\" stroke-width=\"0.25\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol> \n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"stone2\" width=\"2\" height=\"2\" viewBox=\"-1 -1 2 2\" x=\"-1\" y=\"-1\">\n");
	fprintf(f, "  <path d=\"M -0.34 -0.79 Q -0.69 -0.53 -0.73 -0.29 Q -0.73 -0.05 -0.47 0.14 Q -0.33 0.37 0.05 0.33 C 0.53 -0.05 0.62 -0.41 0.16 -0.65 C 0.15 -0.87 -0.23 -0.92 -0.34 -0.79 Z M 0.61 0.22 C 0.44 0.12 0.22 0.28 0.24 0.56 C 0.29 0.88 0.56 0.84 0.69 0.69 C 0.7333 0.5667 0.7767 0.4433 0.61 0.22\" \n");
	fprintf(f, "   fill=\"white\" stroke=\"black\" stroke-width=\"0.25\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"stone3\" width=\"2\" height=\"2\" viewBox=\"-1 -1 2 2\" x=\"-1\" y=\"-1\">\n");
	fprintf(f, "  <path d=\"M -0.57 -0.41 Q -0.89 -0.15 -0.88 0.17 Q -0.77 0.57 -0.27 0.47 Q 0.28 0.58 0.46 0.37 C 0.62 0.23 0.73 -0.1 0.46 -0.41 C 0.28 -0.64 -0.15 -0.85 -0.56 -0.41\" \n");
	fprintf(f, "   fill=\"white\" stroke=\"black\" stroke-width=\"0.25\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"door-floor\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 0 L 3 0 L 3 1.3 L 7 1.3 L 7 0.01 L 10 0 L 10 10 L 7 10 L 7 8.7 L 3 8.7 L 3 10 L 0 10 Z\" \n");
	fprintf(f, "   fill=\"white\" stroke=\"none\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"doorway-floor\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 2 0 L 2 2 L 0 2 L 0 10 L 10 10 L 10 2 L 8 2 L 8 0 Z \" \n");
	fprintf(f, "   fill=\"white\" stroke=\"none\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"doorway\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 0 C 0.46 0.07 0.77 -0.07 2 0 C 2.07 0.27 1.92 0.43 2 2 C 1.43 1.92 1.21 2.03 0 2 C 0.07 2.9 -0.14 4.05 0 10 M 10 10 C 9.83 3.45 10.1 2.91 10 2 C 8.64 1.87 8.51 2.03 8 2 C 8.12 0.62 7.93 0.33 8 0 C 9.21 -0.07 9.31 0.07 10 0 \" \n");
	fprintf(f, "   fill=\"none\" stroke=\"black\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " <symbol id=\"secret-door\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 0 C 0.46 0.07 0.77 -0.07 2 0 C 2.07 0.27 1.92 0.43 2 2 C 1.43 1.92 1.21 2.03 0 2 C 0.07 2.9 -0.14 4.05 0 10 M 10 10 C 9.83 3.45 10.1 2.91 10 2 C 8.64 1.87 8.51 2.03 8 2 C 8.12 0.62 7.93 0.33 8 0 C 9.21 -0.07 9.31 0.07 10 0 M 2 1 C 5 -2 5 4 8 1\" \n");
	fprintf(f, "   fill=\"none\" stroke=\"black\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"door\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 0 0 C 2.1 0.17 2.53 0.12 2.94 0.03 C 2.99 0.51 3.02 0.87 2.99 1.36 C 4.16 1.3 6.05 1.35 7 1.39 C 7.03 0.84 7.01 0.42 6.96 0.04 C 8.22 -0.08 8.95 0.12 10 0 M 10 10 C 7.94 9.96 7.31 9.96 6.93 9.98 C 6.85 9.36 6.83 8.95 6.9 8.51 C 5.04 8.48 4.29 8.53 3.01 8.58 C 3.04 8.93 3.12 9.47 3.09 10.04 C 2.76 10.08 1.26 9.82 0 10 M 3.59 1.35 C 3.58 5.91 3.67 7.09 3.66 8.51 M 6.52 8.46 C 6.51 6.54 6.33 3.25 6.31 1.38 M 3.64 5.09 C 5.1 5.07 5.36 5.12 6.44 5.06\" \n");
	fprintf(f, "   fill=\"none\" stroke=\"black\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol> \n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"stairs\" width=\"12\" height=\"12\" viewBox=\"-6 -6 12 12\">\n");
	fprintf(f, "  <path d=\"M -3.86 -3.8 C -1.31 -3.77 1.61 -3.67 4.23 -3.7 M -1.67 3.19 C -0.45 3.16 0.75 3.19 1.68 3.29 M -3.16 -1.39 C -0.9433 -1.4667 1.0133 -1.4733 3.35 -1.51 M -2.43 0.97 C -0.97 0.86 1.05 0.89 2.45 0.91\" \n");
	fprintf(f, "   fill=\"none\" stroke=\"black\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"cobwebs\" width=\"12\" height=\"12\" viewBox=\"-6 -6 12 12\">\n");
	fprintf(f, "  <path d=\"M -5 -5 M -5 -5 M -5 -5 C -4.51 -3.32 -3.86 -1.71 -3.37 -0.02 M -5 -5 C -4.01 -3.48 -2.8 -2.01 -1.95 -0.71 M -5 -5 C -3.43 -4.04 -2 -2.8 -0.77 -1.97 M -5 -5 C -3.35 -4.62 -1.78 -4.05 -0.21 -3.61 M -4.98 -0.56 C -4.74 -1.19 -4.33 -1.57 -3.75 -1.19 C -3.6 -1.75 -3.37 -1.9 -2.76 -1.85 C -2.79 -2.55 -2.42 -2.69 -1.94 -2.85 C -2 -3.4 -1.72 -3.76 -1.03 -3.86 C -1.5 -4.44 -1.38 -4.78 -0.92 -4.99 M -5.04 -2.65 C -4.85 -3.12 -4.62 -3.12 -4.29 -2.83 C -4.22 -3.16 -4.08 -3.4 -3.69 -3.14 C -3.63 -3.6 -3.49 -3.75 -3.14 -3.76 C -3.3 -4.11 -3.19 -4.34 -2.85 -4.4 C -3.07 -4.72 -3.05 -4.9 -2.67 -5.01\" \n");
	fprintf(f, "   fill=\"none\" stroke=\"black\" stroke-width=\"0.25\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"statue\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 1.58 4.88 C 1.39 3.5 3.32 1.79 5.08 2.06 C 6.56 2.23 7.33 3.04 7.7 4.69 C 7.58 8.01 5.18 7.56 4.9 7.64 C 3.9 7.53 2.22 7.34 1.95 5\" \n");
	fprintf(f, "   fill=\"none\" stroke=\"black\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, "  <path d=\"M 5.01 2 c -0.55 0.38 -0.62 1.84 -0.87 1.83 C 3.19 3.88 1.83 3.76 1.73 3.93 C 1.69 4.36 3.69 5.04 3.48 5.24 C 3.61 5.49 2.87 7.18 3.09 7.37 C 3.67 7.55 4.64 5.92 4.88 5.94 C 5.09 5.7 6.56 7.31 6.8 7.2 C 6.99 6.95 6.14 5.28 6.38 5.2 C 6.66 4.79 7.33 4.5 7.61 3.9 C 7.46 3.82 5.72 4.09 5.68 3.86 C 5.39 3.5 5.26 1.81 5.02 2\" \n");
	fprintf(f, "   stroke=\"none\" fill=\"black\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");


	fprintf(f, " <symbol id=\"crack-t\" width=\"12\" height=\"12\" viewBox=\"-1 -1 12 12\">\n");
	fprintf(f, "  <path d=\"M 4.19 0 C 2.91 1.02 2.79 0.98 2.2 1.49 C 3.22 1.93 3.67 2.25 4.48 2.83 C 4.1 3.17 2.56 3.98 2.6 4.1 C 3.51 6.25 4.04 7.15 4.13 7.04 C 3.84 5.84 3.42 4.98 3.19 4.17 C 4.13 3.65 4.85 3.12 5.26 2.9 C 4.84 2.37 4.35 1.85 3.71 1.36 C 3.87 1.15 5.49 0.46 6.19 0\" \n");
	fprintf(f, "  fill=\"black\" stroke=\"none\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, "\n");
	fprintf(f, " <symbol id=\"pillar\" width=\"10\" height=\"10\" viewBox=\"-3 -3 10 10\" x=\"-3\" y=\"-3\">\n");
	fprintf(f, "  <path d=\"M 0.08 -2.44 C -1.5 -2.35 -1.98 -1.48 -2.19 -0.03 C -2.05 1.32 -1.28 2.02 0.04 2.18 C 1.2 2.07 2.31 1 2.3 0 C 2.16 -1.22 1.65 -2.14 -0.05 -2.3\" \n");
	fprintf(f, "  fill=\"#EEE\" stroke=\"black\" stroke-width=\"0.5\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n");
	fprintf(f, " </defs>\n\n");

	for(i = 0; i < P.n_points; i++) {
		float sx = frand()*0.1 + 0.9;
		float sy = frand()*0.1 + 0.9;
		fprintf(f, " <use href=\"#blobs\" transform=\"translate(%.2f %.2f) rotate(%d) scale(%.2f %.2f)\"/>\n", P.points[i].x, P.points[i].y, ((i*60) + D_RAND(30) - 15)%360, sx, sy);
	}
	for(i = 0; i < P.n_points; i++) {
		float sx = frand()*0.1 + 0.9;
		float sy = frand()*0.1 + 0.9;
		fprintf(f, " <use href=\"#strokes\" transform=\"translate(%.2f %.2f) rotate(%d) scale(%.2f %.2f)\"/>\n", P.points[i].x, P.points[i].y, ((i*60) + D_RAND(30) - 15)%360, sx, sy);
	}
	poisson_done(&P);

	for(j = 0; j < M->map_h; j++) {
		for(i = 0; i < M->map_w; i++) {
			char tile = d_get_tile(M, i,j);
			if(tile == 'D') {
				D_Edge *edge = d_corridor_at(M, i, j);
				assert(edge);
				if(edge->direction == D_ORIENT_EW) {
					fprintf(f, " <use href=\"#door-floor\" transform=\"translate(%d %d)\"/>\n", i * 10, j * 10);
				} else {
					fprintf(f, " <use href=\"#door-floor\" transform=\"translate(%d %d) rotate(90 6 6)\"/>\n", i * 10, j * 10);
				}
			} else if(tile == D_TILE_SECRET){
				int rot = 0, t;
				D_Edge *edge = d_corridor_at(M, i, j);
				assert(edge);
				if(edge->direction == D_ORIENT_EW) {
					if(edge->w == 1) {
						D_Room *r = d_room_at(M, i+1, j);
						assert(r);
						if(r->flags & D_FLAG_SECRET_ROOM)
							rot = -90;
						else
							rot = 90;
					} else if((t = d_get_tile(M, i+1, j)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = 90;
					else if((t = d_get_tile(M, i-1, j)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = -90;
				} else {
					if(edge->h == 1) {
						D_Room *r = d_room_at(M, i, j + 1);
						assert(r);
						if(r->flags & D_FLAG_SECRET_ROOM)
							rot = 0;
						else
							rot = 180;
					} else if((t = d_get_tile(M, i, j - 1)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = 0;
					else if((t = d_get_tile(M, i, j + 1)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = 180;
				}
				fprintf(f, " <use href=\"#doorway-floor\" transform=\"translate(%d %d) rotate(%d 6 6)\"/>\n", i * 10, j * 10, rot);
			} else if(tile == 't') {
				int rot = 0;
				if(d_get_tile(M, i, j + 1) == D_TILE_FLOOR)
					rot = 180;
				if(d_get_tile(M, i+1, j) == D_TILE_FLOOR)
					rot = 90;
				if(d_get_tile(M, i-1, j) == D_TILE_FLOOR)
					rot = -90;
				fprintf(f, " <use href=\"#doorway-floor\" transform=\"translate(%d %d) rotate(%d 6 6)\"/>\n", i * 10, j * 10, rot);
			} else if(!d_is_wall(M, i,j)) {
				fprintf(f, " <use href=\"#floor\" transform=\"translate(%d %d)\"/>\n", i * 10, j * 10);
			}
		}
	}

	for(j = 0; j < M->map_h; j++) {
		for(i = 0; i <M->map_w; i++) {
			char tile = d_get_tile(M, i,j);
			if(d_is_wall(M, i,j))
				continue;

			fprintf(f, " <g transform=\"translate(%d %d)\">\n", i * 10, j * 10);
			if(tile == '@') {
				int rot = 0, t;
				if((t = d_get_tile(M, i, j + 1)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {

					if((t = d_get_tile(M, i+1, j)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
						/* The entrance is a hole in the ceiling */
						fprintf(f, "  <use href=\"#wall-l\"/><use href=\"#wall-r\"/><use href=\"#wall-t\"/>\n");
						rot = 0;
					} else {
						fprintf(f, "  <use href=\"#wall-l\"/><use href=\"#wall-r\"/>\n");
						rot = 0;
					}
				} else if((t = d_get_tile(M, i, j - 1)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					fprintf(f, "  <use href=\"#wall-l\"/><use href=\"#wall-r\"/>\n");
					rot = 180;
				} else if((t = d_get_tile(M, i + 1, j)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					fprintf(f, "  <use href=\"#wall-t\"/><use href=\"#wall-b\"/>\n");
					rot = -90;
				} else if((t = d_get_tile(M, i - 1, j)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					fprintf(f, "  <use href=\"#wall-t\"/><use href=\"#wall-b\"/>\n");
					rot = 90;
				}
				fprintf(f, "  <use href=\"#stairs\" transform=\"rotate(%d 6 6)\"/>\n", rot);
			} else if(tile == '$') {
				int rot = 0, t;
				if((t = d_get_tile(M, i, j + 1)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					if((t = d_get_tile(M, i+1, j)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
						/* The exit is just a hole in the floor */
						fprintf(f, "  <use href=\"#wall-l\"/><use href=\"#wall-r\"/><use href=\"#wall-b\"/>\n");
						rot = 0;
					} else {
						fprintf(f, "  <use href=\"#wall-l\"/><use href=\"#wall-r\"/>\n");
						rot = 180;
					}
				} else if((t = d_get_tile(M, i, j - 1)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					fprintf(f, "  <use href=\"#wall-l\"/><use href=\"#wall-r\"/>\n");
					rot = 0;
				} else if((t = d_get_tile(M, i + 1, j)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					fprintf(f, "  <use href=\"#wall-t\"/><use href=\"#wall-b\"/>\n");
					rot = 90;
				} else if((t = d_get_tile(M, i - 1, j)) == D_TILE_FLOOR || t == D_TILE_FLOOR_EDGE) {
					fprintf(f, "  <use href=\"#wall-t\"/><use href=\"#wall-b\"/>\n");
					rot = -90;
				}
				fprintf(f, "  <use href=\"#stairs\" transform=\"rotate(%d 6 6)\"/>\n", rot);
			} else if(tile == 'S') {
				fprintf(f, "  <use href=\"#statue\" transform=\"rotate(%d 6 6)\"/>\n", D_RAND(360));
			} else if(tile == D_TILE_SECRET) {
				int rot = 0, t;
				D_Edge *edge = d_corridor_at(M, i, j);
				assert(edge);
				if(edge->direction == D_ORIENT_EW) {
					if(edge->w == 1) {
						D_Room *r = d_room_at(M, i+1, j);
						assert(r);
						if(r->flags & D_FLAG_SECRET_ROOM)
							rot = -90;
						else
							rot = 90;
					} else if((t = d_get_tile(M, i+1, j)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = 90;
					else if((t = d_get_tile(M, i-1, j)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = -90;
				} else {
					if(edge->h == 1) {
						D_Room *r = d_room_at(M, i, j + 1);
						assert(r);
						if(r->flags & D_FLAG_SECRET_ROOM)
							rot = 0;
						else
							rot = 180;
					} else if((t = d_get_tile(M, i, j - 1)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = 0;
					else if((t = d_get_tile(M, i, j + 1)) == D_TILE_FLOOR || t == D_TILE_CROSSING)
						rot = 180;
				}
				fprintf(f, " <use href=\"#secret-door\" transform=\"rotate(%d 6 6)\"/>\n", rot);
			} else if(tile == 't') {
				int rot = 0;
				if(d_get_tile(M, i, j + 1) == D_TILE_FLOOR)
					rot = 180;
				if(d_get_tile(M, i + 1, j) == D_TILE_FLOOR)
					rot = 90;
				if(d_get_tile(M, i - 1, j) == D_TILE_FLOOR)
					rot = -90;
				fprintf(f, " <use href=\"#doorway\" transform=\"rotate(%d 6 6)\"/>\n", rot);
			} else if(tile == 'D') {
				D_Edge *edge = d_corridor_at(M, i, j);
				assert(edge);
				if(edge->direction == D_ORIENT_EW) {
					fprintf(f, "  <use href=\"#door\"/>\n");
				} else {
					fprintf(f, "  <use href=\"#door\" transform=\"rotate(90 6 6)\"/>\n");
				}
			} else {
				int r = D_RAND(100);
				if(d_is_wall(M, i - 1, j))
					fprintf(f, "  <use href=\"#wall-l\"/>\n");
				if(d_is_wall(M, i + 1, j))
					fprintf(f, "  <use href=\"#wall-r\"/>\n");
				if(d_is_wall(M, i, j - 1))
					fprintf(f, "  <use href=\"#wall-t\"/>\n");
				if(d_is_wall(M, i, j + 1))
					fprintf(f, "  <use href=\"#wall-b\"/>\n");

				if(r < 4) {
					float sx = frand()*0.1 + 0.9;
					float sy = frand()*0.1 + 0.9;
					fprintf(f, " <use href=\"#stone1\" transform=\"translate(%.2f %.2f) rotate(%d) scale(%.2f %.2f)\"/>\n", frand()*7.0+2.0, frand()*7.0+2.0, D_RAND(360) - 180, sx, sy);
				} else if(r < 8) {
					float sx = frand()*0.1 + 0.9;
					float sy = frand()*0.1 + 0.9;
					fprintf(f, " <use href=\"#stone2\" transform=\"translate(%.2f %.2f) rotate(%d) scale(%.2f %.2f)\"/>\n", frand()*7.0+2.0, frand()*7.0+2.0, D_RAND(360) - 180, sx, sy);
				} else if(r < 12) {
					float sx = frand()*0.1 + 0.9;
					float sy = frand()*0.1 + 0.9;
					fprintf(f, " <use href=\"#stone3\" transform=\"translate(%.2f %.2f) rotate(%d) scale(%.2f %.2f)\"/>\n", frand()*7.0+2.0, frand()*7.0+2.0, D_RAND(360) - 180, sx, sy);
				}

			}
			if(!d_is_wall(M, i + 1, j))
				fprintf(f, "  <use href=\"#floor-r%c\"/>\n",(i+j)%3 + '1');
			if(!d_is_wall(M, i, j + 1))
				fprintf(f, "  <use href=\"#floor-b%c\"/>\n",(i+j)%3 + '1');
			fprintf(f, " </g>\n");
		}
	}

	/* Random cobwebs */
	for(i = 0; i < 3; i++) {
		r = d_random_room(M);
		switch(D_RAND(4)) {
			case 0:
				if(d_get_tile(M, r->x, r->y) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#cobwebs\" transform=\"translate(%d %d)\"/>\n", r->x * 10, r->y * 10);
					d_set_tile(M, r->x, r->y, 'X');
				} else
					i--;
				break;
			case 1:
				if(d_get_tile(M, r->x + r->w - 1, r->y) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#cobwebs\" transform=\"translate(%d %d) rotate(90 6 6)\"/>\n", (r->x + r->w - 1) * 10, r->y * 10);
					d_set_tile(M, r->x + r->w - 1, r->y, 'X');
				} else
					i--;
				break;
			case 2:
				if(d_get_tile(M, r->x + r->w - 1, r->y + r->h - 1) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#cobwebs\" transform=\"translate(%d %d) rotate(180 6 6)\"/>\n", (r->x + r->w - 1) * 10, (r->y + r->h - 1) * 10);
					d_set_tile(M, r->x + r->w - 1, r->y + r->h - 1, 'X');
				} else
					i--;
				break;
			case 3:
				if(d_get_tile(M, r->x, r->y + r->h - 1) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#cobwebs\" transform=\"translate(%d %d) rotate(-90 6 6)\"/>\n", r->x * 10, (r->y + r->h - 1) * 10);
					d_set_tile(M, r->x, r->y + r->h - 1, 'X');
				}
				else
					i--;
				break;
		}
	}

	/* Random cracks */
	for(i = 0; i < 3; i++) {
		r = d_random_room(M);
		switch(D_RAND(4)) {
			case 0:
				j = r->x + D_RAND(r->w);
				if(d_get_tile(M, j, r->y) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#crack-t\" transform=\"translate(%d %d)\"/>\n", j * 10, r->y * 10);
					d_set_tile(M, j, r->y, 'X');
				} else
					i--;
				break;
			case 1:
				j = r->y + D_RAND(r->h);
				if(d_get_tile(M, r->x + r->w - 1, j) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#crack-t\" transform=\"translate(%d %d) rotate(90 6 6)\"/>\n", (r->x + r->w - 1) * 10, j * 10);
					d_set_tile(M, r->x + r->w - 1, j, 'X');
				} else
					i--;
				break;
			case 2:
				j = r->x + D_RAND(r->w);
				if(d_get_tile(M, j, r->y + r->h - 1) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#crack-t\" transform=\"translate(%d %d) rotate(180 6 6)\"/>\n", j * 10, (r->y + r->h - 1) * 10);
					d_set_tile(M, j, r->y + r->h - 1, 'X');
				} else
					i--;
				break;
			case 3:
				j = r->y + D_RAND(r->h);
				if(d_get_tile(M, r->x, j) == D_TILE_FLOOR_EDGE) {
					fprintf(f, " <use href=\"#crack-t\" transform=\"translate(%d %d) rotate(-90 6 6)\"/>\n", r->x * 10, j * 10);
					d_set_tile(M, r->x, j, 'X');
				}
				else
					i--;
				break;
		}
	}

	/* Room with pillars */
	do {
		r = d_random_room(M);
	} while(r->flags & (D_FLAG_START_ROOM | D_FLAG_END_ROOM | D_FLAG_GONE_ROOM) || (r->w * r->h < 8));
	for(i = 1; i < r->w; i++)
		for(j = 1; j < r->h; j++) {
			fprintf(f, " <use href=\"#pillar\" transform=\"translate(%d %d) rotate(%d)\"/>\n", (r->x + i) * 10 + 1, (r->y + j) * 10 + 1, D_RAND(360));
		}

	fprintf(f, "</svg>\n");
}

int main(int argc, char *argv[]) {
	FILE *f;
	int seed;
	D_Dungeon M;

	if(argc > 1)
		seed = atoi(argv[1]);
	else {
		seed = time(NULL);
	}
	srand(seed);

	printf("Seed: %d\n", seed);

	d_init(&M);

	M.grid_w = 4;
	M.grid_h = 4;

	d_generate(&M);

	d_draw(&M, stdout);

	f = fopen("map.svg","w");
	if(f) {
		drawSvg(&M, f);
		fclose(f);
	} else {
		fprintf(stderr, "Unable to save SVG");
	}

	printf("Seed: %d", seed);

	d_deinit(&M);

	return 0;
}
