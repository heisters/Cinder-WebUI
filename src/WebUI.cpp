#include "WebUI.h"
#include "cinder/Log.h"
#include "boost/lexical_cast.hpp"

using namespace ci;
using namespace std;
using namespace webui;

#pragma mark -- Event

Event::Event() :
mType( Type::UNKNOWN )
{

}

Event::Event( const Type &type, const ci::JsonTree &data ) :
mType( type ),
mData( data )
{

}

#pragma mark -- Server
Server::Server() :
WebSocketServer()
{
    addConnectCallback( &Server::onConnect, this );
    addDisconnectCallback( &Server::onDisconnect, this );
    addErrorCallback( &Server::onError, this );
    addInterruptCallback( &Server::onInterrupt, this );
    addPingCallback( &Server::onPing, this );
    addReadCallback( &Server::onRead, this );
}

void Server::onConnect()
{
    CI_LOG_I( "Client connected" );
}

void Server::onDisconnect()
{
    CI_LOG_I( "Client disconnected" );
}

void Server::onInterrupt()
{
    CI_LOG_I( "interrupt" );
}

void Server::onError( string err )
{
    CI_LOG_W( "WebUI error: " << err );
}

void Server::onPing( string msg )
{
}

void Server::onRead( string msg )
{
    JsonTree parsed;
    try
    {
        parsed = JsonTree( msg );
    }

    catch( JsonTree::Exception err )
    {
        CI_LOG_W( "could not parse message: " << msg );
        return;
    }



    for ( const auto &n : parsed )
    {
        string command = n.getKey();
        if ( command == "set" )
        {
            dispatch( Event( Event::Type::SET, parsed.getChild( "set" ) ) );
        }

        else if ( command == "get" )
        {
            dispatch( Event( Event::Type::GET, parsed.getChild( "get" ) ) );
        }
    }
}

Server::EventSignal & Server::getEventSignal( const Event::Type &type )
{
    return mEventSignals[ type ];
}

void Server::get( const string &name )
{
    stringstream ss;
    JsonTree json( "get", name );
    ss << json;
    write( ss.str() );
}

template< typename T >
void Server::set( const string &name, const T &value )
{
    stringstream ss;
    JsonTree json = JsonTree::makeObject();
    json.addChild( JsonTree::makeObject( "set" ) );
    json.getChild( "set" ).addChild( JsonTree( name, value ) );
    ss << json;
    write( ss.str() );
}


void Server::dispatch( const Event &event )
{
    getEventSignal( event.getType() )( event );
}


#pragma mark -- WebUI

WebUI::WebUI()
{
    mServer.getEventSignal( Event::Type::SET ).connect( ::std::bind( &WebUI::onSet, this, ::std::placeholders::_1 ) );

    mServer.getEventSignal( Event::Type::GET ).connect( ::std::bind( &WebUI::onGet, this, ::std::placeholders::_1 ) );
}

void WebUI::update()
{
    mServer.poll();
}

void WebUI::listen( uint16_t port )
{
    mServer.listen( port );
    CI_LOG_I( "WebUI listening on port " << port );
}

WebUI::bound_params_container::iterator WebUI::findParam( const string &name )
{
    return mParams.find( name );
}


struct from_string_visitor : boost::static_visitor<>
{
    from_string_visitor( const string &s ) : str( s ) {};
    string str;

    template< typename bound_param_t >
    void operator()( bound_param_t const &param ) const
    {
        auto v = (*param)();
        *param = boost::lexical_cast< decltype( v ) >( str );
    }
    
};

void WebUI::onSet( Event event )
{
    for ( const auto &n : event.getData() )
    {
        string name = n.getKey();
        auto it = findParam( name );
        if ( it == mParams.end() )
        {
            CI_LOG_W( "unknown param: " << name );
            return;
        }


        auto &p = it->second;
        try
        {
            boost::apply_visitor( from_string_visitor( n.getValue() ), p );
        }

        catch ( boost::bad_lexical_cast err )
        {
            CI_LOG_W( "Could not set param " << name << " with value of " << n.getValue() );
        }
    }
}


struct server_set_visitor : boost::static_visitor<>
{
    server_set_visitor( Server &s, const string &n ) : server( s ), name( n )  {};
    Server &server;
    string name;

    template< typename T >
    void operator()( T const &value )
    {
        server.set( name, *value );
    }
};

void WebUI::onGet( Event event )
{
    string name = event.getData().getValue();
    auto it = findParam( name );
    if ( it == mParams.end() )
    {
        CI_LOG_W( "unknown param: " << name );
        return;
    }


    auto &p = it->second;
    server_set_visitor visitor( mServer, it->first );
    boost::apply_visitor( visitor, p );
}