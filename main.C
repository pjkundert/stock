#include <stock.H>

int main( int, const char ** )
{
    StockTicker			ST;
    int				playerdefaultworth 	= ST_START;

    srandom( (int)time( 0 ) );

    ST.addstock( "Gold" );
    ST.addstock( "Silver" );
    ST.addstock( "Oil" );
    ST.addstock( "Industry" );
    ST.addstock( "Bonds" );
    ST.addstock( "Grain" );


    halfdelay_setup();

#define		GETKEY			0
#define		GETLINE			1
#define		GETNUM			2
#define		INIT			3	// values lower than this are ``input'' states
#define		INITWAIT		4
#define 	RUN			5
#define		COMMAND			6
#define		CMDBANKER		7
#define		CMDBANKERSTOCK		8
#define		CMDBANKERSTOCKADD      10
#define		CMDBANKERSTOCKDELETE   11
#define		CMDPLAYER	       12
#define		CMDPLAYERADD	       13
#define		CMDPLAYERDELETE	       14
#define		CMDPLAYERWORTH	       15
#define		CMDQUIT		       16
#define		DONE		       17
#define		PLAYER		       18
#define		PLAYERBUY	       19
#define		PLAYERMARGIN	       20
#define		PLAYERSELL	       21
#define		PLAYERBUYSTOCK	       22
#define		PLAYERMARGINSTOCK      23
#define		PLAYERSELLSTOCK	       24

    char	        MODSTR[80];
    int			MODCOL;
    int			MODFLG;
    int			CMDCOL	= 0;

    int			y;
    int			key;
    char		line[1024];
    int			linelen;
    player	       *p;
    stock	       *s;
    holding	       *h;
    long long		units;

    int			state[10];
    int			level;

    double		rev;

    // 
    // Scan the revision number, and construct the mode line string
    // 
    sscanf( "$Revision: 1.12 $", "%[ $:a-zA-Z]%lf",
	   MODSTR, &rev );
    sprintf( MODSTR, "Stock Ticker %g: ", rev );
    MODCOL = strlen( MODSTR );
    MODFLG = MODCOL - 2;

#define STATERESET()					\
     (  key		= 0,				 \
     linelen		= 0,				  \
     line[linelen]	= '\0' )
#define STATESET( n )					\
    ( level		= -1,				 \
     STATEPUSH( n ))
#define STATEPUSH( n )					\
    ( state[++level]	= ( n ),			 \
     STATERESET())
#define STATEPOP( n )					\
    ( level = (( n ) > level				 \
	       ? 0				  	  \
	       : level - ( n )))
#define STATE()						\
      ( state[level] )

    mvprintw( ST_CMDLIN, 0, "To start; Command some Players to be Added..." );
    clrtoeol();
    refresh();

    STATESET( INITWAIT );
    while ( STATE() != DONE ) {

	// 
	// Rotate the activity flag only on input states.  Leave cursor at
        // the activity flag.
	if ( STATE() < INIT ) {
	    static char		flag = '/';
	    switch( flag ) {
	      case '/':		flag = '-';	break;
	      case '-':		flag = '\\';	break;
	      case '\\':	flag = '|';	break;
	      case '|':		flag = '/';	break;
	    }
	    mvaddch( ST_MODLIN, MODFLG, flag ); move( ST_MODLIN, MODFLG );
	}

	switch ( STATE() ) {
	  case GETKEY:
	    // 
	    // Get a single key from input, into ''key''.  No echo.
	    // If a ^G is entered, then drop back to the INIT state.
	    // If an ESC is entered, then pop back PAST the invoking
	    // state -- to the state that called IT!
	    // 
	    key = getch();
	    if ( key == KEY_CTRL('G')) {
		STATESET( INIT );
		beep();
	    } else if ( key == KEY_ESC ) {
		STATERESET();
		STATEPOP( 2 );
	    } else {
		STATEPOP( 1 );
	    }
	    break;

	  case GETNUM:
	  case GETLINE:
	    // 
	    // Get a line from input, with echo, into the ''line'' buffer.
	    // linelen is zero on entry...  Pops back to the previous
	    // state on ``newline'' or ``carriage return'', pops back 2 states
	    // on ``escape'', and back to the INIT state on ``CTRL-g''.
	    // 
	    while ( 1 ) {
		// Print the current input line, before waiting for keystroke
		line[linelen]	= '\0';
		mvprintw( ST_CMDLIN, CMDCOL,"%s", line );
		clrtoeol();
		refresh();

		key = getch();
		if ( key		== KEY_BS
		    || key		== KEY_DEL ) {
		    if ( linelen > 0 ) {
			--linelen;
		    }
		} else if ( key		== KEY_RETURN
			   || key	== KEY_NEWLINE ) {
		    STATEPOP( 1 );
		    break;
		} else if ( key		== KEY_ESC ) {
		    STATERESET();
		    STATEPOP( 2 );
		    break;
		} else if ( key 	== KEY_CTRL('G')) {
		    STATESET( INIT );
		    break;
		} else if ( key		< 32
			   || key	> 127 ) {
		    beep();
		} else if ( STATE() == GETNUM
			   && ! isdigit( key )) {
		    //
		    // If we are getting a number, and we see a non-digit,
		    // quit.  If it was a 'k', then add three zeros first.  If
		    // 'm', then 6 zeros!
		    //
		    if ( key == 'k'
			|| key == 'K' ) {
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen]   = '\0';
		    } else if ( key == 'm'
				|| key == 'M' ) {
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen++] = '0';
			line[linelen]   = '\0';
		    }
		    STATEPOP( 1 );
		    break;
		} else {
		    line[linelen++] = key;
		    line[linelen]   = '\0';
		}
	    }
	    break;

	  case INITWAIT:
	    // 
	    // Identical to INIT, except INITWAIT does NOT clear the command
	    // line for a couple of seconds... The RUN state will
	    // re-invoke INIT when getch() returns ERR...  Must turn raw() mode
	    // back on, because halfdelay() turns it off (to avoid having Ctrl-C
	    // exit the program during halfdelay() mode...)
	    // 
	    halfdelay( HALFDELAY_ERROR ); raw();
	    // fall through...

	  case INIT:
	    // attron( A_REVERSE );
	    mvaddstr( ST_MODLIN, 0, MODSTR );
	    // attroff( A_REVERSE );
	    clrtoeol();
	    if ( STATE() != INITWAIT ) {
		move( ST_CMDLIN, 0 );
		clrtoeol();
	    }

	    // 
	    // Clear all the state data collection variables, and redraw
	    // Command and Mode lines.
	    // 
	    p		= NULL;
	    s		= NULL;
	    h		= NULL;
	    units	= -1;

	    STATESET( RUN );
	    break;

	  case RUN:
	    ST.draw();
	    mvprintw( ST_MODLIN, MODCOL, "READY: SPACE)Roll, " );
	    for ( p = ST.players; p; p = p->next ) {
		addch( p->key );
	    }
	    printw( ")Players, CTRL-C)Command" );
	    clrtoeol();
	    refresh();

	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch ( key ) {
	      case ERR:
		// 
		// Must be in half-delay mode due to INITWAIT; invoke INIT
		// to clear the command line...
		// 
		halfdelay_clear();
		STATESET( INIT );
		break;

	      case ' ':
		STATERESET();
		ST.roll();
		break;

	      case KEY_CTRL('C'):
		STATEPUSH( COMMAND );
		break;

	      case KEY_CTRL('L'):
		STATESET( INIT );
		clear();
		break;

	      default:
		if ( ! ST.players
		     || (( p = ST.players->find( key ))
			 == NULL )) {
		    STATERESET();
		    beep();
		} else {
		    halfdelay_clear();
		    STATEPUSH( PLAYER );
		}
		break;
	    }
	    break;

	  case PLAYER:
	    mvprintw( ST_MODLIN, MODCOL, "PLAYER %s: B)uy, M)argin buy, S)ell",
		      p->name );
	    clrtoeol();
	    move( ST_CMDLIN, 0 );
	    clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch ( key ) {
	      case 'b':
	      case 'B':
		STATEPUSH( PLAYERBUY );
		break;

	      case 'm':
	      case 'M':
		STATEPUSH( PLAYERMARGIN );
		break;

	      case 's':
	      case 'S':
		STATEPUSH( PLAYERSELL );
		break;

	      default:
		STATERESET();
		beep();
		break;
	    }
	    break;
	    
	  case PLAYERBUY:
	  case PLAYERMARGIN:
	  case PLAYERSELL:
	    // 
	    // Determine which stock the player wishes to buy or sell
	    // 
	    mvprintw( ST_MODLIN, MODCOL, "PLAYER %s %s: ",
		     p->name, 
		     ( STATE() == PLAYERSELL
		       ? "SELL"
		       : ( STATE() == PLAYERMARGIN
			   ? "MARGIN"
			   : "BUY" )));
	    for ( s = ST.stocks; s; s = s->next ) {
		addch( s->key );
	    }
	    printw( ")Stock" );
	    clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }

	    s = ST.stocks->find( key );
	    if ( s == NULL ) {
		mvprintw( ST_CMDLIN, 0, "No such stock! Try again..." );
		clrtoeol();
		beep();
		refresh();
		STATERESET();
		break;
	    }
	    switch( STATE() ) {
	    case PLAYERBUY:	STATEPUSH( PLAYERBUYSTOCK );	break;
	    case PLAYERMARGIN:	STATEPUSH( PLAYERMARGINSTOCK );	break;
	    default:		STATEPUSH( PLAYERSELLSTOCK );	break;
	    }
	    break;

	  case PLAYERBUYSTOCK:
	  case PLAYERMARGINSTOCK:
	  case PLAYERSELLSTOCK:
	    // 
	    // See if the player is allowed to buy/sell that stock and, if so, how much.  If a valid
	    // amount is entered, perform the transaction.
	    // 
	    h		= p->holdings->find( s->key );
	    assert( h != NULL );

	    // Calculate target units; 0, if in debt.
	    units	= ( STATE() == PLAYERSELLSTOCK
			    ? h->units
			    : ( STATE() == PLAYERBUYSTOCK
				? ((( p->cash / s->value * 100 )
				    / ST_STOCKUNITS )
				   * ST_STOCKUNITS )
				: (((( p->cash + p->margin() ) / s->value * 100 )
				    / ST_STOCKUNITS )
				   * ST_STOCKUNITS )));
	    if ( units < 0 )
		units	= 0;

	    if ( units == 0 ) {
		mvprintw( ST_CMDLIN, 0, "%s %s %s!",
			 p->name, ( STATE() == PLAYERSELLSTOCK
				    ? "has no"
				    : "can't afford" ),
			  s->name );
		clrtoeol();
		beep();
		refresh();
		STATESET( INITWAIT );
		break;
	    }

	    // Now, get the amount to buy/sell, or all
	    mvprintw( ST_MODLIN, MODCOL, "PLAYER %s %s %s: 0->%d)Amount, A)ll",
		     p->name, ( STATE() == PLAYERSELLSTOCK
			       ? "SELL"
			       : ( STATE() == PLAYERMARGINSTOCK
				   ? "MARGIN"
				   : "BUY" )),
		     s->name, units );

	    // 
	    // Figure out what amount was entered (or A for all).  Get another line, if none
	    // entered, or junk.
	    // 
	    long		amount;
	    if (( sscanf( line, "%ld", &amount ) == 1
		 && ( amount % ST_STOCKUNITS != 0
		     || amount > units
		     || amount < 0 ))
		|| ( linelen == 0
		    && key != 'a'
		    && key != 'A' )) {
		if ( linelen ) {
		    // Incorrect value previously entered; beep.
		    beep();
		}
		STATERESET();
		mvprintw( ST_CMDLIN, 0, "Enter amount (divisible by 500): " );
#if defined( USEGETYX )
		getyx( stdscr, y, CMDCOL );
#else
		CMDCOL = 33;
#endif
		refresh();
		STATEPUSH( GETNUM );
		break;
	    }

	    // 
	    // Got the valid amount; now, do the transaction.
	    // 
	    if ( key == 'a' || key == 'A' ) {
		amount = units;
	    }
	    if ( STATE() == PLAYERSELLSTOCK ) {
		p->cash        += amount * s->value / 100;
		h->units       -= amount;
	    } else {
		p->cash	       -= amount * s->value / 100;
		h->units       += amount;
	    }
	    h->value		= h->units * s->value / 100;

	    p->bought		= h->key;			// remember last stock bought/sold

	    mvprintw( ST_CMDLIN, 0, "%s %s %d %s.",
			 p->name, ( STATE() == PLAYERSELLSTOCK
				    ? "sells"
				    : "buys" ),
		      amount, s->name );
	    clrtoeol();
	    refresh();
	    STATESET( INITWAIT );
	    break;

	  case COMMAND:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND: B)ank P)layer Q)uit" );
	    clrtoeol();
	    move( ST_CMDLIN, 0 );
	    clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch ( key ) {
	      case 'b':
	      case 'B':
		STATEPUSH( CMDBANKER );
		break;

	      case 'p':
	      case 'P':
		STATEPUSH( CMDPLAYER );
		break;

	      case 'q':
	      case 'Q':
		STATEPUSH( CMDQUIT );
		break;

	      default:
		STATERESET();
		beep();
		break;
	    }
	    break;

	  case CMDBANKER:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND BANKER: S)tock" ); clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch ( key ) {
	      case 's':
	      case 'S':
		STATEPUSH( CMDBANKERSTOCK );
		break;

	      default:
		STATERESET();
		beep();
		break;
	    }
	    break;

	  case CMDBANKERSTOCK:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND BANKER STOCK: A)dd D)elete" ); clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch ( key ) {
	      case 'a':
	      case 'A':
		STATEPUSH( CMDBANKERSTOCKADD );
		break;

	      case 'd':
	      case 'D':
		STATEPUSH( CMDBANKERSTOCKDELETE );
		break;

	      default:
		STATERESET();
		beep();
		break;
	    }
	    break;

	  case CMDBANKERSTOCKADD:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND BANKER STOCK ADD" ); clrtoeol();
	    mvprintw( ST_CMDLIN, 0, "Enter stock name -->" );
#if defined( USEGETYX )
	    getyx( stdscr, y, CMDCOL );
#else
	    CMDCOL = 20;
#endif

	    refresh();
	    if ( ! linelen ) {
		STATEPUSH( GETLINE );
		break;
	    }

	    ST.addstock( line );
	    STATESET( INIT );
	    break;

	  case CMDBANKERSTOCKDELETE:
	    // 
	    // Determine which stock the player wishes to buy or sell
	    // 
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND BANKER STOCK DELETE <" );
	    for ( s = ST.stocks; s; s = s->next ) {
		addch( s->key );
	    }
	    printw( "> Stock" );
	    clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }

	    s = ST.stocks->find( key );
	    if ( s == NULL ) {
		mvprintw( ST_CMDLIN, 0, "No such stock! Try again..." );
		clrtoeol();
		beep();
		refresh();
		STATERESET();
		break;
	    }

	    for ( p = ST.players; p; p = p->next ) {
		h = p->holdings->find( key );
		if ( h->units != 0 ) {
		    mvprintw( ST_CMDLIN, 0,
			     "%s holds that stock! Try again...",
			     p->name );
		    clrtoeol();
		    beep();
		    refresh();
		    STATERESET();
		    s = NULL;
		    break;
		}
	    }
	    if ( s != NULL ) {
		ST.deletestock( s );
		clear();
		STATESET( INIT );
	    }
	    break;

	  case CMDPLAYER:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND PLAYER: A)dd D)elete W)orth" ); clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch( key ) {
	      case 'a':
	      case 'A':
		STATESET( CMDPLAYERADD );
		break;

	      case 'd':
	      case 'D':
		STATESET( CMDPLAYERDELETE );
		break;

	      case 'w':
	      case 'W':
		STATESET( CMDPLAYERWORTH );
		break;

	      default:
		STATERESET();
		beep();
		break;
	    }
	    break;

	  case CMDPLAYERADD:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND PLAYER ADD" ); clrtoeol();
	    mvprintw( ST_CMDLIN, 0, "Enter player name -->" );
	    refresh();
#if defined( USEGETYX )
	    getyx( stdscr, y, CMDCOL );
#else
	    CMDCOL = 21;
#endif

	    if ( ! linelen ) {
		STATEPUSH( GETLINE );
		break;
	    }

	    ST.addplayer( line, playerdefaultworth );
	    STATESET( INIT );
	    break;

	  case CMDPLAYERDELETE:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND PLAYER DELETE <" ); clrtoeol();
	    for ( p = ST.players; p; p = p->next ) {
		addch( p->key );
	    }
	    printw( "> Player" );
	    clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    p = ST.players->find( key );
	    if ( p == NULL ) {
		mvprintw( ST_CMDLIN, 0, "No such player! Try again..." );
		clrtoeol();
		beep();
		refresh();
		STATERESET();
		break;
	    }

	    delete p;
	    clear();
	    STATESET( INIT );
	    break;

	case CMDPLAYERWORTH:
	    mvprintw( ST_MODLIN, MODCOL, "COMMAND PLAYER WORTH" ); clrtoeol();
	    mvprintw( ST_CMDLIN, 0, "Enter new players' net worth (now %d): ",
		      playerdefaultworth );
	    refresh();
#if defined( USEGETYX )
	    getyx( stdscr, y, CMDCOL );
#else
	    CMDCOL = 32;
#endif

	    // 
	    // See if there's data, and its a number.  If not, get one
	    if ( linelen ) {
	        long		amount;
		if ( sscanf( line, "%ld", &amount ) != 1
		     || amount <= 0 ) {
	            beep();
		    STATERESET();
		} else {
		    // Success!
		    clear();
		    STATESET( INIT );
		    playerdefaultworth = amount;
		    break;
		}
	    }
	    // No data, or no number.  Default to the "average" net worth...
	    STATEPUSH( GETNUM );
	    linelen = snprintf( line, sizeof line, "%d", ST.averageworth() );
	    break;

	  case CMDQUIT:
	    mvprintw( ST_MODLIN, MODCOL, "QUIT: Really? (y/n)" );
	    clrtoeol();
	    refresh();
	    if ( ! key ) {
		STATEPUSH( GETKEY );
		break;
	    }
	    switch ( key ) {
	      case 'y':
	      case 'Y':
		STATEPUSH( DONE );
		break;

	      default:
		beep();
		// fall through...

	      case 'n':
	      case 'N':
		STATESET( INIT );
		break;
	    }
	    break;

	  default:
	    mvprintw( ST_CMDLIN, 0, "Unimplemented..." );
	    clrtoeol();
	    beep();
	    STATESET( INITWAIT );
	    break;
	}
    }

    //
    // Done; StockTicker instance will be destructed here...
    // 
}
