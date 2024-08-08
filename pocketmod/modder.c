/*
Simple program to generate [Pocketmod][] PDFs using [libharu][] .

You can use `-f` to use the [pocketfold][] arrangement.

* Shoutout to [PDFGen][] which was considered as an alternative to [libharu][]
* https://www.mylifeorganized.net/support/pocketmod/
* https://www.overleaf.com/latex/templates/creating-pocketmods-with-latex/nqbhpnrkskrx

[Pocketmod]: https://pocketmod.com/
[libharu]: https://github.com/libharu/libharu
[pocketfold]: https://3skulls.itch.io/pocketfold
[PDFGen]: https://github.com/AndreRenaud/PDFGen/blob/master/pdfgen.h

*/

#include <stdio.h>
#include <math.h>
#include <setjmp.h>

#include <unistd.h>

#include "hpdf.h"

HPDF_Doc pdf;
HPDF_Font font;

static double panelw, panelh;

static int pocketmod[] = {0,1,2,3,4,5,6,7};
static int pocketfold[] = {7,0,1,2,3,4,5,6};
static int *translate = pocketmod;

static int margin = 10;

static int show_pagenums = 0;

void error_handler(HPDF_STATUS   error_no,
					 HPDF_STATUS   detail_no,
					 void         *user_data) {
	fprintf(stderr, "error_handler: %lu %lu\n", error_no, detail_no);
}


static void place_image(HPDF_Page page, int pageno, HPDF_Image image) {

	double angle = 3.141592;
	double x = 0, y = HPDF_Page_GetHeight(page) / 2;

	int mx = -margin, my = -margin;

	int pos = translate[pageno];
	switch(pos) {
		case 0: // Front cover
			x = panelw;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			x = (pos - 1) * panelw;
			mx = margin;
			my = margin;
			angle = 0;
			break;
		case 5:
			x = 4 * panelw;
			break;
		case 6:
			x = 3 * panelw;
			break;
		case 7: // Back cover
			x = 2 * panelw;
			break;
	}

	double cr = cos(angle), sr = sin(angle);
	double wcr = (panelw - 2*margin) * cr,
		wsr = (panelw - 2*margin) * sr,
		hsr = (panelh - 2*margin) * -sr,
		hcr = (panelh - 2*margin) * cr;

	if(image) {
		HPDF_Page_GSave(page);
		HPDF_Page_Concat(page, wcr, wsr, hsr, hcr, x + mx, y + my);
		HPDF_Page_ExecuteXObject(page, image);
		HPDF_Page_GRestore(page);
	}

	char buffer[128];
	snprintf(buffer, sizeof buffer, "%d", pageno);

	if(show_pagenums) {
		double tx = x - panelw/2 + 3, ty = HPDF_Page_GetHeight(page) / 2 - 5;
		switch(pos) {
			case 1:
			case 2:
			case 3:
			case 4:
				tx = x + panelw/2 - 3;
				ty = HPDF_Page_GetHeight(page) / 2 + 5;
				break;
		}
		if((pageno == 0 || pageno == 7) && show_pagenums < 2) return;
		HPDF_Page_BeginText(page);
		HPDF_Page_SetFontAndSize(page, font, 10);
		HPDF_Page_SetTextMatrix(page, cr, sr, -sr, cr, tx, ty);
		HPDF_Page_ShowText(page, buffer);
		HPDF_Page_EndText(page);
	}
}


void usage(FILE *f, const char *name) {
    fprintf(f, "Usage: %s [options] img0 ... img7\n", name);
    fprintf(f, "where options:\n");
    fprintf(f, " -o file.pdf     : Output filename\n");
    fprintf(f, " -f              : Pocketfold layout\n");
    fprintf(f, " -m margin       : Specify margin of each panel\n");
    fprintf(f, " -g              : Fold guide\n");
    fprintf(f, " -p              : Page numbers; `-pp` for page numbers on covers\n");
    fprintf(f, " -T title        : Set document title\n");
    fprintf(f, " -A author       : Set document author\n");
    fprintf(f, " -C creator      : Set document creator\n");
    fprintf(f, " -S subject      : Set document subject\n");
    fprintf(f, " -K keywords     : Set document keywords\n");
}

int main(int argc, char *argv[]) {
	jmp_buf env;

	const char *outfile = "pocketmod.pdf";

	pdf = HPDF_New(error_handler, NULL);
	if(!pdf) {
		fprintf(stderr, "error: Couldn't create PDF object\n");
		return 1;
	}

	if(setjmp(env)) {
		HPDF_Free(pdf);
		return 1;
	}

	const char *T = NULL, *A = NULL, *C = NULL, *S = NULL, *K = NULL;
	int guide = 0;

	int opt;
	while((opt = getopt(argc, argv, "o:fm:T:A:gp?")) != -1) {
        switch(opt) {
             case 'o': outfile = optarg; break;
             case 'f': translate = pocketfold; break;
             case 'm': margin = atoi(optarg); break;
             case 'g': guide++; break;
             case 'p': show_pagenums++; break;
             case 'T': T = optarg; break;
             case 'A': A = optarg; break;
             case 'C': C = optarg; break;
             case 'S': S = optarg; break;
             case 'K': K = optarg; break;
			 default: {
                usage(stdout, argv[0]);
                return 0;
            }
		}
	}

	if(T) HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, T);
	if(A) HPDF_SetInfoAttr(pdf, HPDF_INFO_AUTHOR, A);
	if(C) HPDF_SetInfoAttr(pdf, HPDF_INFO_CREATOR, C);
	if(S) HPDF_SetInfoAttr(pdf, HPDF_INFO_SUBJECT, S);
	if(K) HPDF_SetInfoAttr(pdf, HPDF_INFO_KEYWORDS, K);

	HPDF_Image images[8];
	int ni = 0;
	for(int i = optind; i < argc && ni < 8; i++) {
		HPDF_Image image = HPDF_LoadPngImageFromFile(pdf, argv[i]);
		if(!image) {
			fprintf(stderr, "error: Couldn't load %s", argv[i]);
			return 1;
		}
		images[ni++] = image;
	}

	HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);

	//HPDF_SetPageMode(pdf, HPDF_PAGE_MODE_USE_OUTLINE);

	HPDF_Page page = HPDF_AddPage(pdf);

	HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);
	HPDF_Page_SetLineWidth(page, 0.5);

	panelw = HPDF_Page_GetWidth(page) / 4;
	panelh = HPDF_Page_GetHeight(page) / 2;

	font = HPDF_GetFont (pdf, "Helvetica", NULL);
	HPDF_Page_SetTextRenderingMode (page, HPDF_FILL);
    HPDF_Page_SetRGBFill (page, 0, 0, 0);

	for(int i = 0; i < 8; i++) {
		place_image(page, i, i < ni ? images[i] : NULL);
	}

	if(guide) {
		HPDF_REAL dash1[] = {6,4,2,4};
		HPDF_REAL dash2[] = {3,3};

		HPDF_Page_SetLineWidth (page, 0);
		HPDF_Page_SetRGBStroke (page, 0.5, 0.5, 0.5);
		HPDF_Page_SetDash(page, dash1, 4, 0);
		HPDF_Page_MoveTo (page, HPDF_Page_GetWidth(page)/4, HPDF_Page_GetHeight(page));
		HPDF_Page_LineTo (page, HPDF_Page_GetWidth(page)/4, 0);
		HPDF_Page_MoveTo (page, HPDF_Page_GetWidth(page)/2, HPDF_Page_GetHeight(page));
		HPDF_Page_LineTo (page, HPDF_Page_GetWidth(page)/2, 0);
		HPDF_Page_MoveTo (page, 3*HPDF_Page_GetWidth(page)/4, HPDF_Page_GetHeight(page));
		HPDF_Page_LineTo (page, 3*HPDF_Page_GetWidth(page)/4, 0);
		HPDF_Page_Stroke (page);

		if(translate == pocketmod) {
			HPDF_Page_SetLineWidth (page, 0);
			HPDF_Page_SetRGBStroke (page, 0.5, 0.5, 0.5);
			HPDF_Page_SetDash(page, dash1, 4, 0);
			HPDF_Page_MoveTo (page, 0, HPDF_Page_GetHeight(page)/2);
			HPDF_Page_LineTo (page, HPDF_Page_GetWidth(page)/4, HPDF_Page_GetHeight(page)/2);
			HPDF_Page_MoveTo (page, 3*HPDF_Page_GetWidth(page)/4, HPDF_Page_GetHeight(page)/2);
			HPDF_Page_LineTo (page, HPDF_Page_GetWidth(page), HPDF_Page_GetHeight(page)/2);
			HPDF_Page_Stroke (page);

			HPDF_Page_SetLineWidth (page, 0);
			HPDF_Page_SetRGBStroke (page, 0.3, 0.3, 0.3);
			HPDF_Page_SetDash(page, dash2, 2, 0);
			HPDF_Page_MoveTo (page, HPDF_Page_GetWidth(page)/4, HPDF_Page_GetHeight(page)/2);
			HPDF_Page_LineTo (page, 3*HPDF_Page_GetWidth(page)/4, HPDF_Page_GetHeight(page)/2);
			HPDF_Page_Stroke (page);

		} else {
			HPDF_Page_SetLineWidth (page, 0);
			HPDF_Page_SetRGBStroke (page, 0.5, 0.5, 0.5);
			HPDF_Page_SetDash(page, dash1, 4, 0);
			HPDF_Page_MoveTo (page, 0, HPDF_Page_GetHeight(page)/2);
			HPDF_Page_LineTo (page, HPDF_Page_GetWidth(page), HPDF_Page_GetHeight(page)/2);
			HPDF_Page_Stroke (page);

		}
	}

	HPDF_SaveToFile(pdf, outfile);

	HPDF_Free(pdf);

	printf("Output written to %s\n~ fin ~\n", outfile);

	return 0;
}