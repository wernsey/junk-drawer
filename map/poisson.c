/*
 * Implementation of Poisson Disk Sampling for a series of 2D points.
 *
 * I used this article as reference [Poisson Disk Sampling in Processing][pdsp]
 *
 * [pdsp]: https://sighack.com/post/poisson-disk-sampling-bridsons-algorithm
 *
 * You might want to look at this link for alternative ideas:
 * <https://bost.ocks.org/mike/algorithms/>
 *
 * Author: Werner Stoop
 * CC0 This work has been marked as dedicated to the public domain.
 * https://creativecommons.org/publicdomain/zero/1.0/
 */
#ifndef POISSON_H
#define POISSON_H

struct point {
	float x, y;
};

struct poisson;

typedef int (*poisson_constraint)(struct poisson *P, float x, float y);

struct poisson {
	int	k;
	float r;
	int w, h;
	struct point x0;
	short n_points;
	struct point *points;
	void *data;
	poisson_constraint constraint;
};

void poisson_init(struct poisson *P);

int poisson_plot(struct poisson *P);

void poisson_done(struct poisson *P);

#if defined(POISSON_TEST) && !defined(POISSON_IMPLEMENTATION)
# define POISSON_IMPLEMENTATION
#endif

#ifdef POISSON_IMPLEMENTATION

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#ifndef P_W0
# define P_W0	100
#endif

#ifndef P_H0
# define P_H0	100
#endif

#ifndef P_R0
# define P_R0	5.0
#endif

#ifndef P_K0
# define P_K0	30
#endif

#define N	2

void poisson_init(struct poisson *P) {
	/* These are set to sensible defaults,
	but you're allowed to change them: */

	P->k = P_K0;
	P->r = P_R0;

	P->w = P_W0;
	P->h = P_H0;

	P->x0.x = rand() % P->w;
	P->x0.y = rand() % P->h;

	P->constraint = NULL;
	P->data = 0;

	/* Don't change these: */
	P->n_points = 0;
	P->points = NULL;
}

void poisson_done(struct poisson *P) {
	if(P->points)
		free(P->points);
	P->points = NULL;
	P->n_points = 0;
}

/* Used internally */
struct generator {
	struct poisson *P;

	short *grid;
	short grid_size, grid_w, grid_h;
	short *active_list;
	short n_active;
	float cell_size;
};

static short _p_insert_point(struct generator *G, float x, float y) {
	short index, gx, gy;

	gx = floor(x/G->cell_size);
	gy = floor(y/G->cell_size);

	index = gy * G->grid_w + gx;
	assert(index < G->grid_size);
	G->grid[index] = G->P->n_points;

	assert(G->P->n_points < G->grid_size);
	index = G->P->n_points++;
	G->P->points[index].x = x;
	G->P->points[index].y = y;

	return index;
}

static float _p_frand() {
	return ((float)rand())/RAND_MAX;
}

#define MAX(a,b) ((a) > (b)? (a) : (b))
#define MIN(a,b) ((a) < (b)? (a) : (b))

static float dist(float x0, float y0, float x1, float y1) {
	float dx = x1 - x0, dy = y1 - y0;
	return sqrt(dx * dx + dy * dy);
}

static int _p_is_valid_point(struct generator *G, float px, float py) {
	short gx, gy, i0, i1, j0, j1, i, j;
	struct poisson *P = G->P;

	if(px < 0 || px >= P->w || py < 0 || py >= P->h) return 0;

	if(P->constraint)
		if(!P->constraint(P, px, py))
			return 0;

	gx = floor(px / G->cell_size);
	gy = floor(py / G->cell_size);

	/* Note to self: I did see some cases where the constraint that no two
	 * points should be closer than R is being violated, and I suspect that
	 * what's happening is that they are just placed so that there ends up
	 * being a empty grid cell between them. This implies some misunderstanding
	 * on my part, but I've decided I can live with it. If you can't live with
	 * it, changing the +/-1s below to +/-2 works around the issue.
	 */
	i0 = MAX(gx - 1, 0);
	i1 = MIN(gx + 1, G->grid_w - 1);
	j0 = MAX(gy - 1, 0);
	j1 = MIN(gy + 1, G->grid_h - 1);

	for(i = i0; i <= i1; i++)
		for(j = j0; j <= j1; j++) {
			short t = G->grid[j * G->grid_w + i];
			if(t >= 0) {
				struct point *q = &G->P->points[t];
				if(dist(px, py, q->x, q->y) < P->r)
					return 0;
			}
		}

	return 1;
}

int poisson_plot(struct poisson *P) {
	short i, k;
	struct generator G;

	G.P = P;

	/* Note to self: I found that if you use `floor()` as in the [pdsp][]
	article, then some of the points end up violating the contstraints */
	G.cell_size = ceil((float)P->r / sqrt(N));

	G.grid_w = ceil((float)P->w / G.cell_size) + 1;
	G.grid_h = ceil((float)P->h / G.cell_size) + 1;
	G.grid_size = G.grid_w * G.grid_h;

	P->n_points = 0;

	G.grid = malloc(G.grid_size * sizeof *G.grid);
	if(!G.grid) {
		fprintf(stderr, "no memory\n");
		return 0;
	}
	for(i = 0; i < G.grid_size; i++)
		G.grid[i] = -1;

	G.active_list = malloc(G.grid_size * sizeof *G.active_list);
	if(!G.active_list) {
		free(G.grid);
		fprintf(stderr, "no memory\n");
		return 0;
	}
	G.n_active = 0;

	P->points = malloc(G.grid_size * sizeof *P->points);
	if(!P->points) {
		free(G.grid);
		free(G.active_list);
		fprintf(stderr, "no memory\n");
		return 0;
	}

	i = _p_insert_point(&G, P->x0.x, P->x0.y);
	G.active_list[G.n_active++] = i;

	while(G.n_active > 0) {
		short ax = rand() % G.n_active;
		short a = G.active_list[ax];
		struct point *p = &P->points[a];

		for(k = 0; k < P->k; k++) {
			float th = ((float)(rand() % 360)) * M_PI / 180;
			float rad = (_p_frand() * 2.0 + 1.0) * P->r;

			float nx = p->x + rad * cos(th);
			float ny = p->y + rad * sin(th);

			if(!_p_is_valid_point(&G, nx, ny))
				continue;

			i = _p_insert_point(&G, nx, ny);
			assert(G.n_active < G.grid_size);
			G.active_list[G.n_active++] = i;
			break;
		}
		if(k == P->k) {
			/* No point found - remove the point from the
			active list by replacing it with the last one in the
			list, and decrementing the counter */
			G.active_list[ax] = G.active_list[--G.n_active];
		}
	}
	free(G.grid);
	free(G.active_list);

	return 1;
}

#endif /* POISSON_IMPLEMENTATION */

#ifdef POISSON_TEST

#include <stdio.h>

static int circle_constraint(struct poisson *P, float x, float y) {
	return dist(P->x0.x, P->x0.y, x, y) < 20;
}

int main(int argc, char* argv[]) {
	FILE *f = stdout;
	int i;

	struct poisson P;

	(void)argc; (void)argv;

	srand(time(NULL));

	poisson_init(&P);
	P.r = 3.0;
	P.x0.x = 50;
	P.x0.y = 60;
	P.constraint = circle_constraint;

	if(!poisson_plot(&P)) {
		return 1;
	}

	fprintf(f, "<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\">\n", P.w, P.h);

#if 0
	fprintf(f, " <symbol id=\"strokes\" width=\"30\" height=\"30\" viewBox=\"-15 -15 30 30\" x=\"-15\" y=\"-15\">\n");
	fprintf(f, "   <circle cx=\"0\" cy=\"0\" r=\"%g\" fill=\"red\" opacity=\"0.1\"/>\n", P_R0);
	fprintf(f, "   <circle cx=\"0\" cy=\"0\" r=\"1\" fill=\"black\"/>\n");
	fprintf(f, " </symbol>\n\n");
#else
	fprintf(f, " <symbol id=\"strokes\" width=\"4\" height=\"4\" viewBox=\"-2 -2 4 4\" x=\"-2\" y=\"-2\">\n");
	fprintf(f, "  <path d=\"M-1.5,-1.5 v3 M0,-1.75 v3.5 M1.5,-1.5 v3\" fill=\"none\" stroke-width=\"0.3\" stroke=\"gray\" stroke-linecap=\"round\"/>\n");
	fprintf(f, " </symbol>\n\n");
#endif


	for(i = 0; i < P.n_points; i++) {
		float sx = _p_frand()*0.1 + 0.9;
		float sy = _p_frand()*0.1 + 0.9;
		fprintf(f, " <use href=\"#strokes\" transform=\"translate(%.2f %.2f) rotate(%d) scale(%.2f %.2f)\"/>\n", P.points[i].x, P.points[i].y, rand()%360 - 180, sx, sy);
	}

	fprintf(f, "</svg>\n");

	poisson_done(&P);

	return 0;
}

#endif /* POISSON_TEST */
#endif /* POISSON_H */
