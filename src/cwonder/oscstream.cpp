/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                                                   *
 *  WONDER - Wave field synthesis Of New Dimensions of Electronic music in Realtime  *
 *  http://swonder.sourceforge.net                                                   *
 *                                                                                   *
 *                                                                                   *
 *  Technische Universitšt Berlin, Germany                                           *
 *  Audio Communication Group                                                        *
 *  www.ak.tu-berlin.de                                                              *
 *  Copyright 2006-2008                                                              *
 *                                                                                   *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or modify             *
 *  it under the terms of the GNU General Public License as published by             *
 *  the Free Software Foundation; either version 2 of the License, or                *
 *  (at your option) any later version.                                              *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.       *
 *                                                                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <iostream>

#include "oscstream.h"
#include "liblo_extended.h"
#include "cwonder_config.h"

using std::string;
using std::list;

OSCStream::OSCStream( string name )
{ 
    this->name = name; 
    pingList = new ListOSCPing( "/WONDER/stream/" + name + "/ping" );
}

OSCStream::~OSCStream( )
{ 
    for( clientsIter = begin(); clientsIter != end(); ++clientsIter )
    {
        if( clientsIter->address )
            lo_address_free( clientsIter->address );
    }

    if( pingList )
        delete pingList;
}  



int OSCStream::connect( string host, string port, string name ) 
{ 
    lo_address address = lo_address_new( host.c_str(), port.c_str() );

    // check whether client is already connected
    for( clientsIter = begin(); clientsIter != end(); ++clientsIter )
    {
        if( issame( clientsIter->address, address ) )
        {
            lo_address_free( address );
            return 1;
        }
    }

    // add to list of stream clients
    clients.push_back( OSCStreamClient( host, port, name, address ) );    

    // add to list of pings
    int ret = pingList->add( address, name );

    wonderlog->print( LOG_INFO, "[V-OSCStream" + name + "] connect host=" + ( string ) host
                                + " port=" + ( string ) port );  
    return ret;
}


void OSCStream::disconnect( lo_address a ) 
{ 
    for( clientsIter = begin(); clientsIter != end(); ++clientsIter )
    {
        if( issame( clientsIter->address, a ) )
        {
            if( clientsIter->address )
            {
                pingList->remove( clientsIter->address );
                lo_address_free( clientsIter->address );
                clients.erase( clientsIter );

                wonderlog->print( LOG_INFO, "[V-OSCStream" + name + "] disconnect host=" + ( string ) lo_address_get_hostname( a )
                                            + " port=" + ( string ) lo_address_get_port( a ) );        
                break;
            }
        }
    }
}


void OSCStream::ping( list< OSCStreamClient >& deadClients )
{
    pingList->ping( deadClients );

    //remove dead clients based on lo_address
    std::list< OSCStreamClient >::iterator deadIter;
    for( deadIter = deadClients.begin(); deadIter != deadClients.end(); ++deadIter )
    {
        for( clientsIter = clients.begin(); clientsIter != clients.end(); ++clientsIter )
        {
            if( ( deadIter->address != NULL ) && ( clientsIter->address !=NULL ) && ( deadIter->address == clientsIter->address ) )
            {
                lo_address_free( clientsIter->address );
                clients.erase( clientsIter );
                break;
            }
        }
    }
}


void OSCStream::pong( int pingNum, lo_address from )
{
    pingList->pong( pingNum, from );
}

list< OSCStreamClient >::iterator OSCStream::begin()
{
    return clients.begin();
}


list< OSCStreamClient >::iterator OSCStream::end()
{
    return clients.end();
}

bool OSCStream::hasClients()
{
    return ( ! clients.empty() );
}


void OSCStream::send( const char* path, const char* types, int argc, lo_arg** argv, lo_message msg )
{
    lo_address from;

    for( clientsIter = begin(); clientsIter != end(); ++clientsIter )
    { 
        from = lo_message_get_source( msg );
        if( ! issame( from, clientsIter->address ) )
            forward( path, types, argc, argv, ( *clientsIter ).address );
    }
        
    wonderlog->print( LOG_INFO, "[V-OSCStream" + name + "] forward " + path );
}


void OSCStream::send( const char* types, int argc, lo_arg** argv, lo_message msg )
{
    lo_address from;

    for( clientsIter = begin(); clientsIter != end(); ++clientsIter )
    { 
        from = lo_message_get_source(msg);
        if( ! issame( from, clientsIter->address ) )
            forward( types, argc, argv, clientsIter->address );
    }

    wonderlog->print( LOG_INFO, "[V-OSCStream" + name + "] forward "+ ( string ) &argv[ 0 ]->s );
}
