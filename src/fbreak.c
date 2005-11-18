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

int main(int argp, char *argc[]) {
    FILE *fp_if, *fp_of;
    int i, fbreaks, fstart, fend, fget;
    char buf[128], fn[128], *p, *p2p, *outbuf;
    long fsize;
    
    if ( argp < 2 ) {
       printf ("error: please specify a filename at least\n");
       return 1;
    }

	// open input file
	fp_if = fopen(argc[1], "rb");
	if (fp_if == NULL) {
		printf ("error: failed to open input file\n");
		return 1;
	}

	// copy off "filename".extension part
    strcpy (fn, argc[1]);
    p = strrchr (fn, '.');
    *p = 0;
	
	// get the file size
    fseek (fp_if, 0, SEEK_END);
    fsize = ftell (fp_if);
    rewind (fp_if);

	// figure up the number of file breaks
	fbreaks = (int) ( (fmod(fsize, FILE_OUT_SIZE) > 0) ? (fsize / FILE_OUT_SIZE)+1 : (fsize / FILE_OUT_SIZE) );

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
	    sprintf (buf, "%s%i.%s", fn, i, FILE_OUT_EXT);
	    fp_of = fopen(buf, "wb");
		
		if (fp_of == NULL) {
			printf ("error: failed to open output file (%s)\n", buf);
			return 1;
	    }

	/*
		fwrite(p, 1, fsize, fp_of);
		fclose(fp_of);
		free(p);
		return 0;
	*/
	    
		fend = FILE_OUT_SIZE * i;
		fstart = fend - FILE_OUT_SIZE;
	    
		p2p = p + fstart;
		fget = (( ((int)fsize - fstart) >= FILE_OUT_SIZE ) ? FILE_OUT_SIZE : ((int)fsize - fstart) );
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

    return 0;
}
