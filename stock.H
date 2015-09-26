#include <curses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

// NAME
//     ST_...
// 
// DESCRIPTION
// 
//     Default settings for the game Stock Ticker.
// 
// 
// 

#define ST_MINIMUM	25	// Minimum unit of currency
#define ST_DIVIDEND	100	// Dividend level (real game is 105)
#define ST_STOCKUNITS	500	// Minimum unit of stocks purchased
#define ST_START	10000	// Cash to start
#define ST_PLAYERWIDTH	10	// 10 columns per player
#define ST_INTEREST	10	// 10%/yr interest, charged
#define ST_MARGIN	50	// 50% margin required, or margin call triggered

#define ST_MODLIN	( LINES - 2 )
#define	ST_CMDLIN	( LINES - 1 )

// 
// NAME
//     halfdelay()
//     beep()
//     attrset()
// 
// DESCRIPTION
// 
//     Optionally simulate halfdelay() by setting up a SIGALRM handler that simply ignores the
// interupt but does NOT restart the system call.  This will be used to interrupt getch() after a
// couple of seconds to clear the command line. (-pjk 23jul95: doesn't work...)
// 
//     Simulate beep() by putting a ^G on the screen.
// 
extern "C" int			halfdelay(
				    int			tenths );

#define USEGETYX		1		// Use the getyx macros...
#define HALFDELAY_ERROR		20		// wait 2 seconds before erasing error message
#if ! defined( NOHALFDELAY )
#  define halfdelay_setup()
#  define halfdelay_clear()	{ nocbreak(); raw(); }
#else
#  define halfdelay( n )				\
    alarm(( n ) / 10 )	// tenths of a second
#  define halfdelay_setup()				\
    static struct sigaction sa_sigalrm = {		 \
	(void(*)(...))SIG_IGN, 0, SA_INTERRUPT		  \
    };							   \
    (void) sigaction( SIGALRM, &sa_sigalrm, NULL )
#  define halfdelay_clear()				\
    alarm( 0 )
#endif

#if ! defined( NOBEEP )
#else
#  define beep()					\
    ( addch( '\007' ), refresh())
#endif

#if ! defined( NOATTRSET )
#else
#  undef	attrset( a )
#  undef	attron( a )
#  undef	attroff( a )
#  define	attrset( a )
#  define	attron( a )
#  define	attroff( a )
#endif

// getyx() seems to be busted; when it is fixed, define USEGETYX
#if ! defined( USEGETYX )
#endif


#define KEY_ESC		'\033'
#define KEY_CTRL(c)	(( c ) & 0x1f )
#define KEY_BS		KEY_CTRL('H')
#define KEY_DEL		'\177'
#define KEY_RETURN	KEY_CTRL('M')
#define KEY_NEWLINE	KEY_CTRL('J')

enum action_e {
    up		= 0,
    down	= 1,
    dividend	= 2,
    bust,
    split
};

class stock {
  public:
    stock		       *next;
    stock		      **prev;

  public:
    char		       *name;
    char			key;
    action_e			action;
    int				percentage;
    int				value;

// 
// NAME
//     stock::stock( list, name, key, y, x )
// 
// DESCRIPTION
// 
//     Create a stock, adding it to the given list of stocks.  Assign it the given name, key and
// coordinates.  Key defaults to the first character of the name, and coordinates default to the row
// following the stock at the head of the list.
// 
                                stock(
				    stock	      **l,
				    const char	       *name_,
				    char		k	= 0 )
				{
				    key		= k ? k : *name_;
				    name	= strdup( name_ );

				    // Capitalise the ``key'' char in ``name''
				    for ( char *n = name; *n; ++n ) {
					*n = tolower( *n );
				    }
				    for ( char *n = name; *n; ++n ) {
					if ( *n == tolower( key )) {
					    *n = toupper( *n );
					    break;
					}
				    }

				    value	= 100;

				    prev	= l;	// prepend to list
				    next	= *l;
				    if ( next ) {
					next->prev = &next;
				    }
				    *l		= this;
				}


			       ~stock()
				{
				    *prev	= next;
				    if ( next ) {
					next->prev	= prev;
				    }
				}

    void			roll();
    void			draw(
				    int			y,
				    int			x );
    stock		       *find(
				    char	        k );
    stock		       *operator[](
				    int			n );

};



// 
// NAME
// 
//     class holding
// 
// DESCRIPTION
// 
//     Stores all the players holdings of a certain type.  The holdings are linked together in a
// sort of doubly linked list, where the holding::prev member points back at the actual pointer that
// points to the holding.  This is to allow a holding to be deleted.
// 
//     Each holding knows its location on the screen.
// 
class holding {
  public:
    char			key;
    holding		      **prev;
    holding		       *next;
    long long			units;
    long long			value;

                                holding(
				    holding           **list,
				    char		k	= 0 )
				{
				    key		= k;
				    units	= 0;
				    value	= 0;

				    prev	= list;
				    next 	= *list;
				    if ( next ) {
					next->prev = &next;
				    }
				    *list      	= this;
				}

                               ~holding() {
				    *prev	= next;
				    if ( next ) {
					next->prev	= prev;
				    }
			        }

    holding		       *find(
				    char		k	= 0 )
    {
				    if ( k == key ) {
					return this;
				    } else if ( next ) {
					return next->find( k );
				    }

				    return NULL;
				}
		
    void			draw(
				    int			x,
				    int			y );
};

class player {
  public:
    player		      **prev;

    char		       *name;
    char		        key;
    player		       *next;
    holding		       *holdings;

    char			bought;		// last stock bought (first sold in margin call...)
    long long			interest;	// interest outstanding (charged when >= 25 (ST_MINIMUM))
    long long			cash;		// how much cash/debt do you have
    long long			worth;

                                player(
				    player	      **list,
				    const char	       *name_,
				    char		k	= 0,
				    int                 start   = ST_START )
				{
				    name	= strdup( name_ );
				    key		= k;

				    // Capitalise the ``key'' char in ``name''
				    for ( char *n = name; *n; ++n ) {
					*n = tolower( *n );
				    }
				    for ( char *n = name; *n; ++n ) {
					if ( *n == tolower( key )) {
					    *n = toupper( *n );
					    break;
					}
				    }
				    
				    prev	= list;
				    next	= *list;
				    if ( next ) {
					next->prev = &next;
				    }
				    *list	= this;

				    bought	= 0;
				    interest	= 0;
				    cash	= start;
				    worth	= start;
				    holdings    = NULL;
				}

    // 
    // player::margin	-- return the total margin available to borrow against
    // 
    //     Multiplies current value of all holdings by the maximum margin allowed (50%) Then,
    // returns values divisible by minimum units of currency.
    // 
    long long			margin() {
				    long long		total 	= 0;
				    for ( holding *h = holdings; h; h = h->next ) {
					total	+= h->value;
				    }
				    return total * ST_MARGIN / 100 / ST_MINIMUM * ST_MINIMUM;
    				}

                               ~player()
				{
				    while ( holdings ) {
					delete holdings;
				    }

				    *prev	= next;
				    if ( next ) {
					next->prev	= prev;
				    }
				}

    player		       *find(
				    char	k	= 0 );

};

class StockTicker {
  public:
                                StockTicker();
                               ~StockTicker();
    void			roll();
    void			run();
    void			addstock(
				    const char	       *name,
				    char		key	= 0 );
    void			deletestock(
				    stock	       *s );
    void			addplayer(
				    const char	       *name,
				    int			start	= ST_START,
				    char		key	= 0 );
    int				averageworth();
    void			draw();

    stock		       *stocks;
    int				stockCount;

    player		       *players;
    int				playerCount;
};

