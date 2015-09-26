#include <stock.H>

// 
// NAME
//     player::find
// 
// DESCRIPTION
//     Finds a player with the given key.
// 
player *
player::find(
    char			k )
{
    if ( k == key ) {
	return this;
    } else if ( next ) {
	return next->find( k );
    }
    
    return NULL;
}

