#include <stock.H>

// 
//////////////////////////////////////////////////////////////////////////////
// 
// NAME
//     class stock
// 

// 
// NAME
//     stock::find( key )
//     stock::find( num )
//
// DESCRIPTION
//     Find the stock matching the given key, or the given number.  If not
// found, return NULL.
//
stock *
stock::find(
    char		       k )
{
    if ( k == key ) {
	return this;
    } else if ( next ) {
	return next->find( k );
    }

    return NULL;
}

stock *
stock::operator[](
    int				n )
{
    if ( n == 0 ) {
	return this;
    } else if ( next ) {
	return (*next)[ n - 1 ];
    }

    return NULL;
}

// 
// NAME
//     stock::draw()
// 
// DESCRIPTION
//     Draw the stock's value, at its assigned location
// 
void
stock::draw(
    int				y,
    int				x )
{
    mvprintw( y, x, "%8.8s: %3d: ", name, value );
    for ( int i = 0; i < 200; i += 5 ) {
	addch( i == value
	      ? '*'
	      : ( i == 100
		 ? '|'
		 : '-' ));
    }
}


// 
// NAME
//     stock::roll()
// 
// DESCRIPTION
// 
//     Roll the dice for this stock.  Figure out up, down, dividend, bust or split actions, and
// change the stock value if necessary.  Leave the results in the action, percentage, and value
// members.
// 
void
stock::roll()
{

    switch ( random() % 3 ) {
      case 0:	percentage	= 5;		break;
      case 1:	percentage	= 10;		break;
      default:	percentage	= 20;		break;
    }

    switch ( random() % 3 ) {
      case 0:
	action		= up;
	value	       += percentage;
	break;

      case 1:
	action		= down;	
	value	       -= percentage;
	break;

      default:
	action		= dividend;
	break;
    }

    if ( value <= 0 ) {
	action	= bust;
	value	= 100;
    } else if ( value >= 200 ) {
	action	= split;
	value	= 100;
    }
}
