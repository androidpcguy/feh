/* feh_initpre.c

Copyright (C) 2012      Christopher Hrabak        LastUpdt Jan 8, 2013 cah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


June 2012  Hrabak rewrote the keyevents.c logic.
See newkeyev.h, feh_initpre.c, feh_init.c and feh_init.h for more detail.

* This is the feh_initpre.c module.  It creates the stubptr.c module which
* is required by the feh_init.c module.  Makefile compiles this to a small
* standalone routine AND RUNS it before compiling the feh_init.c module.
*
* Originally Hrabak used a bash script to do this (pre) feh_init stuff, but
* moved it all into feh_initpre.c, encapsulating the logic in one place.
*
*/

#include "feh.h"
#include "feh_init.h"


int main( void ){
		/* this is a quick-and-dirty to generate the stubptr.c module
		* used by feh_init.c and newkeyev.h.
		* All the logic is in main().  Crude!
		*/

	static char    buf[ DFLT_KB_CNT  ] ;   /* store action names */
	SimplePtrmap fname[ DFLT_KB_CNT  ];
	char action[256];
	int i, j, slen, uniq_cnt=0, foundit=0;
	char  *ptr, *buf_ptr, *tmp_ptr;
	FILE *fs_c=NULL, *fs_h=NULL;          /* _c for stubptr.c, _h for newkeyev.h */


	/* first loop dflt_kb[] to determine how many reMapped functions.
	* reMap funcptr have two ':' per binding , so count them all.
	*/
	hlp.rm_cnt = 0;
	for ( i=0; i<hlp.dflt_cnt; i++ ){
		tmp_ptr = dflt_kb[i];
		while (*tmp_ptr++)
				if (*tmp_ptr == ':' ) hlp.rm_cnt++;
	}
	hlp.rm_cnt -= hlp.dflt_cnt;

	/* store all the reMapped function ptrs in the hlp.remaps buffer,
	* Use the hlp.rm_ptr to follow the trailing '\0'.
	*/
	i = hlp.rm_cnt * 48 ;
	hlp.remaps = malloc( sizeof(char) * i );
	memset(hlp.remaps,'\0', i );
	hlp.rm_ptr = hlp.remaps;
	hlp.rmlist = malloc( sizeof(char*) * hlp.rm_cnt );
	hlp.rm_cnt = 0;         /* from now on it will be the REAL cnt */

	/* loop dflt_kb[] to create a unique set of action names */
	ZERO_OUT( buf );
	buf_ptr=buf;
	for ( i=0; i<hlp.dflt_cnt; i++ ){
			strcpy(action, dflt_kb[i]);
			if ( ( ptr = strchr(action, ':')) )
					*ptr = '\0';
			else {
					printf("feh_init: Oops!  dflt_kb[ %d ] looks bad (%s)", i, dflt_kb[i]);
					continue;
			}
			/* check if we have a reMapped funcptr */
			tmp_ptr = strchr( ptr+1, ':' );
			if (tmp_ptr) {
					/* there is a reMap function.  Save it in hlp.rmlist[] */
					*tmp_ptr++ = '\0';            /* truncates key */
					(*hlp.rmlist)[hlp.rm_cnt++] = hlp.rm_ptr ;    /* what about dupes? */
					hlp.rm_ptr = stpcpy( hlp.rm_ptr, action);
					hlp.rm_ptr++ ;                /* embed the NULL inside the pair */
					hlp.rm_ptr = stpcpy( hlp.rm_ptr, tmp_ptr );
					hlp.rm_ptr++ ;                /* move beyond the last '\0' */
			}
			/* find that action in fname[], else add it */
			slen = ptr - action;
			foundit = 0;
			for (j=0; j<uniq_cnt ; j++ ){
					if ( fname[j].len != slen ) continue;       /* first check string length */
					if ( strcmp( fname[j].action, action ) ==0 ) {
								foundit = 1;
								break;
					}
			}

			if ( foundit == 0 ) {
					fname[uniq_cnt].len    = slen;
					fname[uniq_cnt].action = buf_ptr;
					buf_ptr = stpcpy(buf_ptr, action );
					buf_ptr++;             /* step past the trailing '\0' */
					uniq_cnt++;
			}
	}

	/* fname[] has a unique list of actions.  Sort it */
	qsort( fname ,uniq_cnt, sizeof(SimplePtrmap),  cmp_fname );

	/* loop thru fname[] and create the stubptr.c module.
	* Note: stubptr.c gets compiled into feh_init.c
	* Everything in newkeyev.h is generated here.
	*/

	if ( ( fs_h=fopen( "newkeyev.h", "w" )) == NULL ){
			printf("feh_init: Could not open newkeyev.h");
			exit(1);
	}

	if ( ( fs_c=fopen( "stubptr.c", "w" )) == NULL ){
			printf("feh_init: Could not open stubptr.c");
			exit(1);
	}

	fprintf( fs_c, "/* stubptr.c - auto generated by feh_init.c */" );
	/* fprintf( fs_c, "\n\n#ifdef SECOND_PASS" ); */
	fprintf( fs_c, "\n\n/* TEST functions for ALL stub_actions() */\n\n" );
	fprintf( fs_c, "\n#include <stdio.h>\n\n" );

	/* make the test stub_functions ...
	* Note: The stub_<named> functions are prototyped as int stub_??(int);
	* for  feh_init.c test_all_funcptr(), but void stub_??(void); for the
	* actual newkeyev.c keypress processing.  See line 179 here.
	*/
	for ( j=0; j < uniq_cnt ; j++ ){
			fprintf( fs_c, "int stub_%s(int i){ printf(\" index is %%d, <stub_%s>\\n\",i); return ++i;};\n",
							fname[j].action, fname[j].action );
	}

	/* write out the fname[] array */
	fprintf( fs_c, "\n/* array that maps the action name to the index ( parallels funcptr[] ) */\n");
	fprintf( fs_c, "typedef struct {\n");
	fprintf( fs_c, "  char * action;\n");
	fprintf( fs_c, "  int len;\n");
	fprintf( fs_c, "} SimplePtrmap;\n");
	fprintf( fs_c, "\nextern SimplePtrmap fname[];\n\n");
	fprintf( fs_c, "SimplePtrmap fname[] = {\n");
	for ( j=0; j < uniq_cnt ; j++ ){
			fprintf( fs_c, "{ \"%s\" , %d } ,\n", fname[j].action, fname[j].len );
	}
	fprintf( fs_c, "};  /* end of fname[] */\n");
	fprintf( fs_c, "\nint fname_cnt=%d ;\n", uniq_cnt );   /* for -DSECOND_PASS */

	/* fprintf( fs_c, "\n#endif       - SECOND_PASS is defined - \n" ); */

	/* create the newkeyev.h header from scratch ... */
	fprintf( fs_h, "/* newkeyev.h \n\n");
	fprintf( fs_h, "* June 2012 Note:  This header is autogenerated by \n");
	fprintf( fs_h, "*    feh_init.c -DFIRST_PASS \n*\n");
	fprintf( fs_h, "* Please read feh_init.h for the \"why\" of it all.\n*\n");
	fprintf( fs_h, "* ANY CHANGES MADE TO THIS FILE WILL BE LOST !\n*\n*/\n\n");
	fprintf( fs_h, "#ifndef NEWKEYEV_H\n");
	fprintf( fs_h, "#define NEWKEYEV_H\n\n");

	/* make the prototypes ... */
	fprintf( fs_h, "\n\n/* stub_function prototypes for newkeyev.c */\n\n" );
	for ( j=0; j < uniq_cnt ; j++ ){
			fprintf( fs_h, "void stub_%s( void );\n", fname[j].action );
	}

	/* make the funcptr[] array ...
	* This is written to BOTH fs_c and fs_h file streams, and is
	* included in BOTH ( FIRST & SECOND ) versions of feh_init.c.
	*
	* report the remap list like ...
	*              ret2pos ==> stub_move_ret2pos
	*/
	fprintf( fs_c, "\n/* The rest of this is included in BOTH versions of feh_init */\n");
	fprintf( fs_c, "/* List of all actions ReMapped to other functions...        */\n" );

	fprintf( fs_h, "\n/* List of all actions ReMapped to other functions...        */\n" );

	for (i=0; i< hlp.rm_cnt; i++ ) {
			hlp.rm_ptr = strchr( (*hlp.rmlist)[i], '\0');
			fprintf( fs_c, "/* %20s ==> %-30s   */\n", (*hlp.rmlist)[i], hlp.rm_ptr+1 );
			fprintf( fs_h, "/* %20s ==> %-30s   */\n", (*hlp.rmlist)[i], hlp.rm_ptr+1 );
	}

	fprintf( fs_c, "\n\n /* funcptr[] array is used by both feh_init.c and newkeyev.c */" );
	fprintf( fs_c, "\n\ntypedef  int (*StubPtr)( int );      /* def for function pointer */\n\n" );
	fprintf( fs_c, "\n\nStubPtr funcptr[] = { \n" );

	fprintf( fs_h, "\n\n /* funcptr[] array is used by both feh_init.c and newkeyev.c */" );
	fprintf( fs_h, "\n\nStubPtr funcptr[] = { \n" );

	foundit = 0;
	for ( j=0; j < uniq_cnt ; j++ ){
			for (i=0; i< hlp.rm_cnt; i++ ){
					foundit = 0;
					if ( strcmp( (*hlp.rmlist)[i], fname[j].action) == 0 ){
						hlp.rm_ptr = strchr( (*hlp.rmlist)[i], '\0');
						fprintf( fs_c, " &%s ,    /* ReMapped*/\n", hlp.rm_ptr+1  );
						fprintf( fs_h, " &%s ,    /* ReMapped*/\n", hlp.rm_ptr+1  );
						foundit = 1;
						break;
					}
			}
			if ( !foundit ){
					fprintf( fs_c, " &stub_%s ,\n", fname[j].action );
					fprintf( fs_h, " &stub_%s ,\n", fname[j].action );
			}
	}
	fprintf( fs_c, "};  /* end of funcptr[] */\n\n");
	fprintf( fs_h, "};  /* end of funcptr[] */\n\n");

	fprintf( fs_c, "\n\n#define DFLT_ACT_CNT    %d\n\n", uniq_cnt);
	fprintf( fs_h, "\n\n#define DFLT_ACT_CNT    %d\n\n", uniq_cnt);

	/* fprintf( fs_c, "\n#ifndef SECOND_PASS\n" ); */

	/* make a function to test-call all the function pointers */
	fprintf( fs_c, "\n\nvoid test_all_funcptr( void ){\n\n");
	fprintf( fs_c, "   int i=0;\n\n    for (i=0; i< DFLT_ACT_CNT ; i++ ) funcptr[i]( i );\n\n}\n\n");
	/* fprintf( fs_c, "\n#endif     - SECOND_PASS - \n\n" ); */
	fprintf( fs_c, "/* ############ EVERYTHING ABOVE IS AUTO GENNERATED ############# */ \n");

	fclose(fs_c);

#if 0   /* moved to newkeyev.c */
		/* Function prototypes for newkeyev.c ... */
		fprintf( fs_h, "\n\n/* Non-stub_ type function prototypes for newkeyev.c ... */\n");
		fprintf( fs_h, "void edit_caption( KeySym keysym, int state );\n");
		fprintf( fs_h, "void feh_event_invoke_action( unsigned char action);\n");
		fprintf( fs_h, "feh_node * ret2stack( int pp, int direction ); \n");
		fprintf( fs_h, "void fillFEPnodes( feh_node * center );\n");
		fprintf( fs_h, "void init_FEP( void );\n");
		fprintf( fs_h, "void fillFEPimages( void);\n");
		fprintf( fs_h, "void fill_one_FEP( Zone z, int y);\n");
		fprintf( fs_h, "void make_rn_image( void );\n");
		fprintf( fs_h, "int get_pos_from_user( LLMD *md) ;\n");
		fprintf( fs_h, "StubPtr getPtr4actions( unsigned int symstate, int which, Key2Action actions[] ) ; \n");
		fprintf( fs_h, "StubPtr getPtr4feh( unsigned int symstate ) ; \n");
#endif

	fprintf( fs_h, "\n/* ############ EVERYTHING ABOVE IS AUTO GENNERATED ############# */ \n");
	fprintf( fs_h, "\n#endif    /* end of #ifdef  NEWKEYEV_H */\n");
	fclose(fs_h);

	printf("... feh_initpre completed:  stubptr.c and newkeyev.h created.\n\n");

	return 0;

}   /* end of main() */

int cmp_fname( const void *left, const void *right ){
		/* called by qsort to put fname[] in action name order */
		const SimplePtrmap *l= ( const SimplePtrmap *) left;
		const SimplePtrmap *r= ( const SimplePtrmap *) right;
		return (int)( strcmp( (char *)l->action , (char *)r->action ));
}