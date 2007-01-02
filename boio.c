/*
 *------------------------------------------------------------------------
 * vim: ts=4 sw=4
 *------------------------------------------------------------------------
 * Author:   reynolds (Tommy Reynolds)
 * Filename: boio.c
 * Created:  2007-01-01 10:57:04
 *------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <boio.h>

static	int			fd_qty;				/* Counts FD's we have open			*/
static	boio_t		raw_init =	{
	.fd = -1,
	.map = MAP_FAILED,
};

static	char *
xstrdup(
	char const *	s
)
{
	char * const	result = strdup( s );

	assert( result );
	return( result );
}

static	int
close_backing_store(
	boio_t * const		boio
)
{
	int					result;

	result = -1;
	do	{
		if( boio->fd != -1 )	{
			if( close( boio->fd ) )	{
				/* Some error closing. Should log for debugging				*/
			}
			boio->fd = -1;
			--fd_qty;
		}
		result = 0;
	} while( 0 );
	return( result );
}

static	int
open_backing_store(
	boio_t * const		boio
)
{
	int					result;

	result = -1;
	do	{
		struct stat		st;

		boio->fd = open( boio->fn, boio->flags, boio->mode );
		if( boio->fd == -1 )	{
			/* Log failure to open and/or create the file					*/
			break;
		}
		++fd_qty;
		if( fstat( boio->fd, &st ) )	{
			/* Log failure to stat the file									*/
			close_backing_store( boio );
			break;
		}
		boio->filesize = st.st_size;
		/*																	*/
		result = 0;
	} while( 0 );
	return( result );
}

int
boio_putbuffer(
	boio_t * const		boio
)
{
	int					result;

	result = -1;
	do	{
		if( boio->map != MAP_FAILED )	{
			if( !munmap( boio->map, boio->length ) )	{
				result = 0;
			}
			boio->map = MAP_FAILED;
		}
	} while( 0 );
	return( result );
}

boio_t *
boio_new(
	void
)
{
	boio_t * const	result = malloc( sizeof( *result ) );
	assert( result );
	return( result );
}

boio_t *
boio_open(
	boio_t *		boio,
	char const *	fn,
	int				flags,
	mode_t			mode
)
{
	boio_t *		result;
	boio_t *		working;

	result = NULL;
	working = boio;
	if( !working )	{
		working = boio_new();
	}
	do	{
		/* Do all preparation in a working descriptor, not the real thing	*/
		working = boio;
		if( !working )	{
			working = boio_new();
		}
		*working = raw_init;
		working->fn = xstrdup( fn );
		working->flags = flags;
		working->mode = mode;
		/* Open backing store, creating it if necessary						*/
		if( open_backing_store( boio ) )	{
			/* Log failure to open and/or create							*/
			break;
		}
		if( working->fd > BOIO_FD_MAX )	{
			close_backing_store( boio );
		}
		/* Done initializing, all is OK, so return real results				*/
		result = working;
	} while( 0 );
	if( !result && working )	{
		if( boio )	{
			/* Caller allocated the cookie									*/
			boio_close( working );
		} else	{
			/* We allocated this cookie										*/
			working = boio_free( working );
		}
	}
	return( result );
}

boio_t *
boio_close(
	boio_t * const		boio
)
{
	if( boio )	{
		boio_putbuffer( boio );
		close_backing_store( boio );
		if( boio->fn )	{
			free( boio->fn );
			boio->fn = NULL;
		}
		*boio = raw_init;
	}
	return( boio );
}

boio_t *
boio_free(
	boio_t * const		boio
)
{
	if( boio )	{
		boio_close( boio );
		free( boio );
	}
	return( NULL );
}

static	int
prot_flags(
	boio_t * const		boio
)
{
	int					result;

	switch( boio->flags )	{
	default:
		result = 0;
		break;
	case O_RDONLY:
		result = PROT_READ;
		break;
	case O_WRONLY:
		result = PROT_WRITE;
	case O_RDWR:
		result = (PROT_READ | PROT_WRITE);
	}
	return( result );
}

void *
boio_getbuffer(
	boio_t * const		boio,
	off_t const			offset,
	size_t const		length
)
{
	void *				result;

	result = NULL;
	do	{
		/* Must have access to the backing store for this					*/
		if( open_backing_store( boio ) )	{
			/* Log failure to access backing store							*/
			break;
		}
		do	{
			off_t const	needed = offset + length;

			/* Must have room in the file for this buffer's worth			*/
			if( boio->filesize < needed )	{
				/* Nope, extend if we're writing							*/
				if( boio->flags & (O_WRONLY|O_RDWR) )	{
					boio->filesize += (needed - boio->filesize);
					if( ftruncate( boio->fd, boio->filesize ) )	{
						/* Log failure to extend the file					*/
						break;
					}
				}
			}
			/* Create the buffer map, but adjust for VM page alignment		*/
			boio->align = offset % getpagesize();
			boio->length = length + boio->align;
			boio->offset = offset - boio->align;
			boio->map = mmap(
				NULL,
				boio->length,
				prot_flags( boio ),
				MAP_SHARED,
				boio->fd,
				boio->offset
			);
			if( boio->map == MAP_FAILED )	{
				/* Failure to map buffer could be EOF if reading...			*/
				break;
			}
			/* Success, the file content <=> map has been established		*/
			result = boio->map + boio->align;
		} while( 0 );
		if( boio->fd >= BOIO_FD_MAX )	{
			close_backing_store( boio );
		}
	} while( 0 );
	return( result );
}

size_t
boio_bufferlength(
	boio_t * const		boio
)
{
	return( boio->length - boio->align );
}

off_t
boio_getfilesize(
	boio_t * const		boio
)
{
	return( boio->filesize );
}

off_t
boio_getoffset(
	boio_t * const		boio
)
{
	return( boio->offset + boio->align );
}

char const *
boio_getfilename(
	boio_t * const		boio
)
{
	return( boio->fn );
}
