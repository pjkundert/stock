#include <stock.H>

// 
// NAME
//     holding::draw
// 
// DESCRIPTION
// 
//     Draws the number of units in a particular holding.  If zero, print a dash.  If too big for 8
// columns, print in exponential form.
// 
void
holding::draw(
    int				y,
    int				x )
{
    if ( units ) {
	if ( units > 9999999 ) {
	    mvprintw( y, x, "%9.3g%c", (double)units, key );
	} else {
	    mvprintw( y, x, "%9d%c", (int)units, key );
	}
    } else {
	mvprintw( y, x, "      -- %c", key );
    }
}

