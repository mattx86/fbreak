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
	int opt_r_val;
	FILE *fp_if, *fp_of;
	int i, fbreaks, fstart, fend, fget;
	char buf[128], fn[128], *p, *p2p, *outbuf;
	long fsize;

	if ( argp < 2 ) {
		printf ("fbreak%s, Copyright (c) 2005, Matt Smith\n", VERSION);
		printf ("syntax: %s [-r] [-v] file\n\n", argc[0]);
		printf ("\t-r parts\trebuild file from parts\n");
		printf ("\t-v\t\tverify integrity (to-do)\n");

		return 1;
	}

	opt_r = opt_v = false;
	for (i = 0; i < argp; i++) {
		if ( !strcmp ("-r", argc[i]) && !opt_r ) {
			opt_r_val = atoi(argc[++i]);
			opt_r = true;
		}
		else if ( !strcmp ("-v", argc[i]) && !opt_v ) {
			opt_v = true;
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
		fbreaks = (int) ( (fmod(fsize, OUTPUT_FILE_SIZE) > 0) ? (fsize / OUTPUT_FILE_SIZE)+1 : (fsize / OUTPUT_FILE_SIZE) );

		// allocate memory for input file
		p = (char *) malloc (fsize);
		if (p == NULL) {
			printf ("error: failed to allocate %l bytes for input\n", fsize);
			return 2;
		}
		
		// store it in our buffer and close file
		fread (p, 1, fsize, fp_if);
		fclose (fp_if);
		
		// create a file for each break
		for (i = 1; i <= fbreaks; i++)
		{
			sprintf (buf, "%s%s%i", fn, OUTPUT_FILE_EXT, i);
			fp_of = fopen(buf, "wb");
			
			if (fp_of == NULL) {
				printf ("error: failed to open output file (%s)\n", buf);
				return 1;
			}

			fend = OUTPUT_FILE_SIZE * i;
			fstart = fend - OUTPUT_FILE_SIZE;
			
			p2p = p + fstart;
			fget = (( ((int)fsize - fstart) >= OUTPUT_FILE_SIZE ) ? OUTPUT_FILE_SIZE : ((int)fsize - fstart) );
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

	}

	// rebuild file from parts
	else
	{

		// remove "-partN" from filename
		//p = strrchr(fn, '-');
		//*p = 0;

		fp_of = fopen (fn, "wb");
		if (fp_of == NULL) {
			printf ("error: failed to open output file (%s)\n", fn);
			return 1;
		}

		// rebuild file
		for (i = 1; i <= opt_r_val; i++) {
			sprintf (buf, "%s%s%i", fn, OUTPUT_FILE_EXT, i);
			fp_if = fopen(buf, "rb");
			
			if (fp_if == NULL) {
				printf ("error: failed to open input file (%s)\n", buf);
				return 1;
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
	}

	return 0;
}
