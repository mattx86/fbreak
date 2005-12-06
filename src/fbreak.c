/*
Copyright (c) 2005, Matt Smith
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the distri-
     bution.

  3. Neither the copyright holders nor the contributors names may be
     used to endorse or promote products derived from this software
     without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fbreak.h"

int main(int argp, char **argc) {
	int opt_b, opt_r;
	int i, fbreaks;
	long fsize, fstart, fend, fget, b, bget, breaksize = PARTS_DEFAULT_BREAKSIZE, blocksize = PARTS_DEFAULT_BLOCKSIZE;
	char buf[128], fn[128], *p;
	FILE *fp_if, *fp_of;

	if ( argp < 2 ) {
		printf ("fbreak%s, Copyright (c) 2005, Matt Smith\n", VERSION);
		printf ("syntax: %s [-b blocksizeX] [breaksizeX] file\n", argc[0]);
		printf ("        %s [-b blocksizeX] -r file\n\n", argc[0]);
		printf ("\t-b blocksizeX\tblock size (X=B,K,M,G)\n");
		printf ("\t-r\t\trebuild file from parts\n");
		printf ("\tbreaksizeX\tbreak size (X=B,K,M,G)\n");
		printf ("\tfile\t\tfile to break or rebuild\n");

		return 1;
	}

	opt_b = opt_r = false;
	for (i = 0; i < argp; i++) {
		if ( !strcmp ("-b", argc[i]) && !opt_b ) {
			char blocksize_s[32];
			strcpy (blocksize_s, argc[++i]);
			blocksize = fsize_bytes (blocksize_s);
			opt_b = true;
		}
		else if ( !strcmp ("-r", argc[i]) && !opt_r ) {
			opt_r = true;
		}
		else if ( argp >= 3 && i == (argp - 2) && !opt_r) { // next-to-last option
			char breaksize_s[32];
			strcpy (breaksize_s, argc[i]);
			breaksize = fsize_bytes (breaksize_s);
		}
		else if ( i == (argp - 1) ) // trailing option
			strcpy(fn, argc[i]);
	}

	// break file into parts
	if (!opt_r)
	{

		// open input file
		fp_if = fopen(fn, "rb");
		if (fp_if == NULL) {
			printf ("error: failed to open input file\n");
			return 1;
		}
		
		// get the file size
		fseek (fp_if, 0, SEEK_END);
		fsize = ftell (fp_if);
		rewind (fp_if);

		// sanity check
		if ( breaksize >= fsize || breaksize == 0 || fsize == 0 ) {
			printf ("error: either \"%s\" is empty or the break size (%i B)\nis larger than this file (%i B)\n", fn, breaksize, fsize);
			return 1;
		}

		// figure up the number of file breaks
		fbreaks = (int) (fsize/breaksize);
		if ( fbreaks < ((double)fsize/(double)breaksize) )
			fbreaks++;
		
		printf ("breaking \"%s\" into %i parts...\n", fn, fbreaks);

		// create a file for each break
		for (i = 1; i <= fbreaks; i++)
		{
			sprintf (buf, "%s%s%i", fn, PARTS_EXT, i);
			fp_of = fopen(buf, "wb");
			
			if (fp_of == NULL) {
				printf ("error: failed to open output file (%s)\n", buf);
				return 1;
			}

			fend = breaksize * i;
			fstart = fend - breaksize;
			
			fseek (fp_if, fstart, SEEK_SET);
			fget = (( (fsize - fstart) >= breaksize ) ? breaksize : (fsize - fstart) );
			b = 0;
			while (b != fget) {
				bget = ((blocksize > (fget - b)) ? (fget - b) : blocksize);
				p = (char *) malloc (bget);
				if (p == NULL) {
					printf ("error: failed to allocate %i bytes for output\n", bget);
					return 2;
				}
				fread (p, 1, bget, fp_if);
				fwrite(p, 1, bget, fp_of);
				free  (p);
				b += bget;
				fseek (fp_if, (fstart + b), SEEK_SET);
			}
			fclose (fp_of);
		}

		printf ("done.\n");
		fclose (fp_if);
	}

	// rebuild file from parts
	else
	{

		// remove "-partN" from filename
		if ( strstr (fn, PARTS_EXT) != NULL ) {
			p = strrchr(fn, '-');
			*p = 0;
		}

		fp_of = fopen (fn, "wb");
		if (fp_of == NULL) {
			printf ("error: failed to open output file (%s)\n", fn);
			return 1;
		}

		printf ("rebuilding \"%s\" from parts...\n", fn);

		// rebuild file
		for (i = 1; i <= PARTS_MAX; i++) {
			sprintf (buf, "%s%s%i", fn, PARTS_EXT, i);
			fp_if = fopen(buf, "rb");
			
			if (fp_if == NULL) {
				if ( i == 1 ) {
					printf ("error: failed to open input file (%s)\n", buf);
					return 1;
				}
				else
					break;
			}

			// get the file size
			fseek (fp_if, 0, SEEK_END);
			fsize = ftell (fp_if);
			rewind (fp_if);

			b = 0;
			while (b != fsize) {
				bget = ((blocksize > (fsize - b)) ? (fsize - b) : blocksize );
				p = (char *) malloc (bget);
				if (p == NULL) {
					printf ("error: failed to allocate %l bytes for input\n", bget);
					return 2;
				}
				
				fread (p, 1, bget, fp_if);
				fwrite(p, 1, bget, fp_of);
				free  (p);
				b += bget;
				fseek (fp_if, b, SEEK_SET);
			}
			fclose(fp_if);
		}

		printf ("done.\n");
		fclose (fp_of);
	}

	return 0;
}

long fsize_bytes (char *str) {
	long bytes;
	char m, *p;

	p = str + strlen(str)-1;
	m = *p;

	if ( m == 'G' ) { // Gibibytes (GiB)
		*p = 0; bytes = labs( ((long) (atof(str) * 1024 * 1024 * 1024)) );
	} else if ( m == 'M' ) { // Mebibytes (MiB)
		*p = 0; bytes = labs( ((long) (atof(str) * 1024 * 1024)) );
	} else if ( m == 'K' ) { // Kibibytes (KiB)
		*p = 0; bytes = labs( ((long) (atof(str) * 1024)) );
	} else { // default: Bytes (B)
		if ( m == 'B' )
			*p = 0;
		bytes = labs( atol(str) );
	}

	return bytes;
}