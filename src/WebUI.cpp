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
    } catch ( JsonTree::ExcJsonParserError err ) {
        CI_LOG_W( "Could not parse JSON: " << msg << " " << err.what() );
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

Server::event_signal & Server::getEventSignal( const Event::Type &type )
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

JsonTree makeSetJSON( const JsonTree &valueJSON )
{
    JsonTree json = JsonTree::makeObject();
    json.addChild( JsonTree::makeObject( "set" ) );
    json.getChild( "set" ).addChild( valueJSON );
    return json;
}

template< typename T >
void Server::set( const string &name, const T &value )
{
    stringstream ss;
    JsonTree json = makeSetJSON( JsonTree( name, value ) );
    ss << json;
    write( ss.str() );
}

void Server::set( const string &name, const vec2 &value )
{
    stringstream ss;
    JsonTree valueJSON = JsonTree::makeArray( name );
    valueJSON.addChild( JsonTree( "", value.x ) );
    valueJSON.addChild( JsonTree( "", value.y ) );
    JsonTree json = makeSetJSON( valueJSON );
    ss << json;
    write( ss.str() );
}

void Server::set( const string &name, const vec3 &value )
{
    stringstream ss;
    JsonTree valueJSON = JsonTree::makeArray( name );
    valueJSON.addChild( JsonTree( "", value.x ) );
    valueJSON.addChild( JsonTree( "", value.y ) );
    valueJSON.addChild( JsonTree( "", value.z ) );
    JsonTree json = makeSetJSON( valueJSON );
    ss << json;
    write( ss.str() );
}

void Server::set( const std::string &name, const ci::Colorf &value )
{
    stringstream ss;
    JsonTree valueJSON = JsonTree::makeArray( name );
    valueJSON.addChild( JsonTree( "", value.r ) );
    valueJSON.addChild( JsonTree( "", value.g ) );
    valueJSON.addChild( JsonTree( "", value.b ) );
    JsonTree json = makeSetJSON( valueJSON );
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
    auto it = mParams.find( name );
    if ( it == mParams.end() )
    {
        CI_LOG_W( "unknown param: " << name );
    }

    return it;
}

struct set_from_json_visitor : boost::static_visitor<>
{
    set_from_json_visitor( const JsonTree &j ) : json( j ) {};
    JsonTree json;

    template< typename T >
    void operator()( T const &param_ptr ) const
    {
        auto v = (*param_ptr)();
        param_ptr->set( json.getValue< decltype( v ) >(), false );
    }

    void operator()( BoundParam< vec2 >* const &param_ptr ) const
    {
        vec2 v( json.getChild( 0 ).getValue< float >(),
                json.getChild( 1 ).getValue< float >() );
        param_ptr->set( v, false );
    }

    void operator()( BoundParam< vec3 >* const &param_ptr ) const
    {
        vec3 v( json.getChild( 0 ).getValue< float >(),
                json.getChild( 1 ).getValue< float >(),
                json.getChild( 2 ).getValue< float >() );
        param_ptr->set( v, false );
    }

    void operator()( BoundParam< Colorf >* const &param_ptr ) const
    {
        Colorf v( json.getChild( 0 ).getValue< float >(),
                  json.getChild( 1 ).getValue< float >(),
                  json.getChild( 2 ).getValue< float >() );
        param_ptr->set( v, false );
    }
};

struct server_set_visitor : boost::static_visitor<>
{
    server_set_visitor( Server &s, const string &n ) : server( s ), name( n )  {};
    Server &server;
    string name;

    template< typename T >
    void operator()( T const &value )
    {
        server.set( name, (*value)() );
    }
};

void WebUI::setClients( const bound_params_container::value_type &pair )
{
    server_set_visitor visitor( mServer, pair.first );
    boost::apply_visitor( visitor, pair.second );
}

void WebUI::setSelf( const bound_params_container::value_type &pair, const JsonTree &value )
{
    try
    {
        boost::apply_visitor( set_from_json_visitor( value ), pair.second );
    }

    catch ( JsonTree::Exception err )
    {
        CI_LOG_W( "Could not set param " << pair.first << " with value value of " << value << ". " << err.what() );
    }
}


void WebUI::onSet( Event event )
{
    for ( const auto &n : event.getData() )
    {
        string name = n.getKey();
        auto it = findParam( name );
        if ( it == mParams.end() ) continue;

        setSelf( *it, n );
    }
}



void WebUI::onGet( Event event )
{
    string name = event.getData().getValue();
    auto it = findParam( name );
    if ( it == mParams.end() ) return;

    setClients( *it );
}

void WebUI::onParamChange( const string &name )
{
    auto it = findParam( name );
    if ( it == mParams.end() ) return;

    setClients( *it );
}