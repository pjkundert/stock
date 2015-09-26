#include <stock.H>

StockTicker::StockTicker()
{
    stocks	= NULL;
    stockCount	= 0;

    players	= NULL;
    playerCount	= 0;

    (void)initscr();
    raw();
    noecho();
    nonl();

    srandom( (unsigned int)time( NULL ));
}

void
StockTicker::addstock(
    const char		       *name,
    char			key )
{

    if ( stocks && stocks->find( key )) {
	key = 0;
    }
    if ( ! key ) {
	for ( const char *k = name; *k; ++k ) {
	    if ( ! stocks || ! stocks->find( tolower( *k ))) {
		key = tolower( *k );
		break;
	    }
	}
    }

    // 
    // Search for last handle in list, and add new stock there.
    stock		      **s;
    for ( s = &stocks; *s; s = &(*s)->next )
      ;
    new stock( s, name, key );
    ++ stockCount;

    // 
    // Add a holding for this stock, for each player.
    player		       *p;
    for ( p = players; p; p = p->next ) {
	new holding( &p->holdings, (*s)->key );
    }
    clear();
}

void
StockTicker::deletestock(
    stock		       *s )
{
    player		       *p;

    for ( p = players; p; p = p->next ) {
	holding *h = p->holdings->find( s->key );
	p->cash += h->value;
	delete h;
    }

    delete s;
    -- stockCount;
}

void
StockTicker::addplayer(
    const char		       *name,
    int				start,
    char			key )
{
    player	      	       *p;
    stock		       *s;

    if ( players && players->find( key )) {
	key = 0;
    }
    if ( ! key ) {
	for ( const char *k = name; *k; ++k ) {
	    if ( ! players || ! players->find( tolower( *k ))) {
		key = tolower( *k );
		break;
	    }
	}
    }

    p = new player( &players, name, key, start );
    ++ playerCount;

    // 
    // Add holdings for each of the existing stocks
    // 
    for ( s = stocks; s; s = s->next ) {
	holding		       *h;
	h = new holding( &p->holdings, s->key );
    }
}

int
StockTicker::averageworth()
{
    int				avg	= 0;
    int				count	= 0;

    for ( player *p = players; p; p = p->next, ++count ) {
        avg = ( avg * count + p->worth ) / ( count + 1 );
    }
    return avg - avg % ST_MINIMUM;
}

void
StockTicker::draw()
{
    int				x;
    int				y;
    int				yo;


    // 
    // Print all the stocks, starting from the top of the screen.
    y	= 0;
    yo	= 0;
    for( stock *s = stocks; s; s = s->next ) {
        s->draw( y + yo++, 20 );
    }

    //
    // Print the label of each holding.  Assume every player's holdings
    // are in the same order.
    y = ST_MODLIN - stockCount - 5;
    yo = 1;
    mvprintw( y + yo++, 0, "Net Worth:" );
    mvprintw( y + yo++, 0, "Cash/Loan:" );
    mvprintw( y + yo++, 0, "Margin:" );
    for ( stock *s = stocks; s; s = s->next ) {
	mvprintw( y + yo++, 0, "%s:", 
		 s->name );
    }

    // 
    // Now, print the players.
    x = ST_PLAYERWIDTH;
    for ( player *p = players; p; p = p->next ) {
	yo = 0;
	mvprintw( y + yo++, x, "%10.10s", p->name );
	if ( p->worth > 9999999 )
	    mvprintw( y + yo++, x, "%9.3g=", (double)p->worth );
	else
	    mvprintw( y + yo++, x, "%9d=", (int)p->worth );
	if ( p->cash < - p->margin() ) {
	    attron( A_REVERSE );
	    beep();
	} else if ( p->cash < - p->margin() * 90 / 100 ) {
	    attron( A_BLINK );
	}
	if ( p->cash > 9999999 || p->cash < -999999 )
	    mvprintw( y + yo++, x, "%9.3g$", (double)p->cash );
	else
	    mvprintw( y + yo++, x, "%9d$", (int)p->cash );
	attroff( A_BLINK );
	attroff( A_REVERSE );
	long long	margin	= p->margin();
	if ( margin > 9999999 )
	    mvprintw( y + yo++, x, "%9.3g$", (double)margin );
	else
	    mvprintw( y + yo++, x, "%9d$", (int)margin );
	for ( stock *s = stocks; s; s = s->next ) {
	    if ( p->holdings ) {
		holding *h = p->holdings->find( s->key );
		if ( h ) {
		    if ( h->key == p->bought
			 && p->cash < - p->margin() )
			attron( A_REVERSE );		// This is the one we'll try to sell first...
		    h->draw( y + yo++, x );
		    attroff( A_REVERSE );
		}
	    }
	}
	x += ST_PLAYERWIDTH;
    }
}

void
StockTicker::roll()
{
    stock	       *s;
    holding	       *h;
    player	       *p;

    // 
    // If the player has more loan then margin, and chooses to roll, we have to do a "margin call"
    // on the player's loan.  Pick the highest remaining stock, and sell units to bring his net
    // worth up to zero.  Sell one unit at a time (I'm too lazy right now to figure out the right
    // number of units to sell, to bring loan below cash + margin...)
    // 
    for ( p = players; p; p = p->next ) {
	// 
	// Add 1 "per diem" of interest to loan.
	// 
	if ( p->cash < 0 ) {
	    p->interest	       += (long long)( double( ST_INTEREST ) / 100 / 365	// "per diem"
					       * (( - p->cash ) + p->interest ));	// x loan plus interest
	    if ( p->interest >= ST_MINIMUM ) {
		p->cash	       -= ( p->interest / ST_MINIMUM ) * ST_MINIMUM;
		p->interest    %= ST_MINIMUM;
	    }
	}

	// 
	// Force margin calls, if necessary
	// 
	while ( p->cash < - p->margin() ) {
	    h 			= p->holdings->find( p->bought );
	    if ( h && h->value ) {
		// There is units left in last stock purchased; sell some
		// mvprintw( LINES / 2, 0, "h->value: %d / h->units: %d == %d \n", (int)h->value, (int)h->units,
		//           (int)( h->value * ST_STOCKUNITS / h->units ));
		p->cash        += h->value * ST_STOCKUNITS / h->units;
		h->value       -= h->value * ST_STOCKUNITS / h->units; 
		h->units       -= ST_STOCKUNITS;
	    } else {
		// Nothing left in last stock purchased; look at them all
		for ( h = p->holdings; h; h = h->next ) {
		    if ( h->value ) {
			// mvprintw( LINES / 2, 0, "h->value: %d / h->units: %d == %d \n", (int)h->value, (int)h->units,
			//           (int)( h->value * ST_STOCKUNITS / h->units ));
			p->cash        += h->value * ST_STOCKUNITS / h->units;
			h->value       -= h->value * ST_STOCKUNITS / h->units;
			h->units       -= ST_STOCKUNITS;
			break;			// Found a stock to sell; units of 500...
		    }
		}
	    }
	    if ( ! h )
		break;				// No more stock to sell!  Give up.
	}
    }

    //
    // Now, roll dice on a random stock.
    // 
    s = (*stocks)[random() % stockCount];
    assert( s != NULL );

    s->roll();

    switch( s->action ) {
      case up:
	mvprintw( LINES - 1, 0, "%s up %d", s->name, s->percentage );
	clrtoeol();
	break;
      case down:	      
	mvprintw( LINES - 1, 0, "%s down %d", s->name, s->percentage );
	clrtoeol();
	break;
      case dividend:
	mvprintw( LINES - 1, 0, "%s dividend %d", s->name, s->percentage );
	clrtoeol();
	break;
      case split:
	mvprintw( LINES - 1, 0, "%s SPLITS", s->name );
	beep();
	clrtoeol();
	break;
      case bust:
	mvprintw( LINES - 1, 0, "%s BUSTS", s->name );
	beep();
	clrtoeol();
	break;
    }

    // 
    // Update each player's holdings, according to the new stock valuation, or dividend.
    // 
    for ( p = players; p; p = p->next ) {
	h	      		= p->holdings->find( s->key );

	if ( h != NULL ) {
	    switch( s->action ) {
	      case dividend:
		if ( s->value >= ST_DIVIDEND ) {
		    p->cash	       += h->units * s->percentage / 100;
		}
		break;
	      case bust:
		h->units	= 0;
		break;
	      case split:
		h->units       *= 2;
		break;
	      case up:
	      case down:
		break;
	    }

	    h->value		= h->units * s->value / 100;
	}

	for ( p->worth = p->cash, h = p->holdings; h; h = h->next ) {
	    p->worth	       += h->value;
	}
    }
}

StockTicker::~StockTicker()
{
    move( LINES - 1, 0 );
    refresh();
    endwin();
}

