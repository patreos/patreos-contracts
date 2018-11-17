#include "patreosnexus.hpp"

using namespace eosio;

void patreosnexus::subscribe( name from, name to )
{

}

void patreosnexus::unsubscribe( name from, name to )
{

}


void patreosnexus::pledge( name from, name to, uint32_t seconds, asset quantity )
{

}


void patreosnexus::pledgepaid( name from, name to )
{

}


void patreosnexus::unpledge( name from, name to )
{

}


void patreosnexus::depledge( name from, name to )
{

}


void patreosnexus::setprofile( name owner, patreosnexus::profile sprofile )
{

}


void patreosnexus::unsetprofile( name owner )
{

}


void patreosnexus::publish( name owner, patreosnexus::publication spublication )
{

}

EOSIO_DISPATCH( patreosnexus, (subscribe)(unsubscribe)(pledge)(pledgepaid)(unpledge)(depledge)(setprofile)(unsetprofile)(publish) )
