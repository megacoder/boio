/*
 *------------------------------------------------------------------------
 * vim: ts=4 sw=4
 *------------------------------------------------------------------------
 * Author:   reynolds (Tommy Reynolds)
 * Filename: boio.h
 * Created:  2007-01-01 10:57:10
 *------------------------------------------------------------------------
 */

#ifndef	BOIO_H
# define BOIO_H

# include <sys/types.h>

/* Configuration constants													*/

# define BOIO_FD_MAX	(500)			/* Qty of fd's opened at once		*/

/* Library clients must treat this as a opaque data structure, or else!		*/

typedef	struct boio_s	{
	int				fd;					/* Where backing store is opened	*/
	int				flags;				/* Flags for open(2) call			*/
	mode_t			mode;				/* Mode for open(2) call			*/
	off_t			offset;				/* File offset of current window	*/
	off_t			filesize;			/* How big file currently is		*/
	size_t			length;				/* Length of current window			*/
	off_t			align;				/* To align to VM pagesize			*/
	char *			map;				/* mmap(2) return value				*/
	char *			fn;					/* Name of backing store			*/
} boio_t;

boio_t *		boio_new( void );
boio_t *		boio_open( boio_t *, char const *, int, mode_t );
boio_t *		boio_close( boio_t * );
boio_t *		boio_free( boio_t * );

void *			boio_getbuffer( boio_t *, off_t, size_t );
int				boio_putbuffer( boio_t * );
size_t			boio_bufferlength( boio_t * );
off_t			boio_getfilesize( boio_t * );
off_t			boio_getoffset( boio_t * );
char const *	boio_getfilename( boio_t * );

#endif	/* BOIO_H */
