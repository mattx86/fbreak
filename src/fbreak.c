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
#include <math.h>
#include "fbreak.h"

int main(int argp, char **argc) {
	int opt_r, opt_v;
	FILE *fp_if, *fp_of;
	int i, fbreaks, fstart, fend, fget;
	char buf[128], fn[128], *p, *p2p, *outbuf;
	long fsize, breaksize = PARTS_DEFAULT_SIZE;

	if ( argp < 2 ) {
		printf ("fbreak%s, Copyright (c) 2005, Matt Smith\n", VERSION);
		printf ("default action is to break file\n\n");
		printf ("syntax: %s [-r] [-v] [sizeX] file\n\n", argc[0]);
		printf ("\t-r\t\trebuild file from parts\n");
		printf ("\t-v\t\tverify integrity (to-do)\n\n");
		printf ("\tsizeX\t\tbreak size (X=B,K,M,G  default size: ~1.4M)\n");
		printf ("\tfile\t\tfile to break or rebuild\n");

		return 1;
	}

	opt_r = opt_v = false;
	for (i = 0; i < argp; i++) {
		if ( !strcmp ("-r", argc[i]) && !opt_r ) {
			opt_r = true;
		}
		else if ( !strcmp ("-v", argc[i]) && !opt_v ) {
			opt_v = true;
		}
		else if ( argp >= 3 && i == (argp - 2) && !opt_r) { // next-to-last option
			char breaksize_s[32], m;
			strcpy (breaksize_s, argc[i]);
			p = breaksize_s + strlen(breaksize_s)-1;
			m = *p;

			if ( m == 'G' ) { // Gibibytes (GiB)
				*p = 0; breaksize = (long) (atof(breaksize_s) * 1024 * 1024 * 1024);
			} else if ( m == 'M' ) { // Mebibytes (MiB)
				*p = 0; breaksize = (long) (atof(breaksize_s) * 1024 * 1024);
			} else if ( m == 'K' ) { // Kibibytes (KiB)
				*p = 0; breaksize = (long) (atof(breaksize_s) * 1024);
			} else { // default: Bytes (B)
				if ( m == 'B' )
					*p = 0;
                breaksize = atol(argc[i]);
			}
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

		// figure up the number of file breaks
		fbreaks = (int) ( (fmodl(fsize, breaksize) > 0) ? (fsize / breaksize)+1 : (fsize / breaksize) );

		// allocate memory for input file
		p = (char *) malloc (fsize);
		if (p == NULL) {
			printf ("error: failed to allocate %l bytes for input\n", fsize);
			return 2;
		}
		
		// store it in our buffer and close file
		fread (p, 1, fsize, fp_if);
		fclose (fp_if);
		
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
			
			p2p = p + fstart;
			fget = (( ((int)fsize - fstart) >= breaksize ) ? breaksize : ((int)fsize - fstart) );
			outbuf = (char *) malloc (fget);
			if (outbuf == NULL) {
				printf ("error: failed to allocate %i bytes for output\n", fget);
				return 2;
			}
			memcpy (outbuf, p2p, fget);
			
			fwrite (outbuf, 1, fget, fp_of);
			fclose (fp_of);
			free (outbuf);
		}
		free (p);

		printf ("done.\n");
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

			// allocate memory for input file
			p = (char *) malloc (fsize);
			if (p == NULL) {
				printf ("error: failed to allocate %l bytes for input\n", fsize);
				return 2;
			}
			
			// store it in our buffer, write it, close input file, free buffer memory
			fread (p, 1, fsize, fp_if);
			fwrite(p, 1, fsize, fp_of);
			fclose(fp_if);
			free (p);
		}
		fclose (fp_of);

		printf ("done.\n");
	}

	return 0;
}
