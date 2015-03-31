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
    CI_LOG_V( "ping: " << msg );
}

void Server::onRead( string msg )
{
    CI_LOG_V( "read: " << msg );

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


#pragma mark -- BaseUI

BaseUI::BaseUI()
{
}

void BaseUI::update()
{
    mServer.poll();
}

void BaseUI::listen( uint16_t port )
{
    mServer.listen( port );
    CI_LOG_I( "WebUI listening on port " << port );
}


#pragma mark -- ParamUI::Param

ParamUI::Param::Param( const string &name, float *ptr ) :
mName( name ),
mPtr( ptr )
{

}

ParamUI::ParamOptions & ParamUI::Param::getOptions()
{
    return mOptions;
}

struct from_string_visitor : boost::static_visitor<>
{
    from_string_visitor( const string &s ) : str( s ) {};
    string str;

    template< typename T >
    void operator()( T const &value ) const
    {
        auto v = *value; // convert to rvalue: "if the value category of expression is lvalue, then the decltype specifies T&"
        *value = boost::lexical_cast< decltype( v ) >( str );
    }

};

struct to_string_visitor : boost::static_visitor<>
{
    to_string_visitor() : str( "" ) {};
    string str;

    template< typename T >
    void operator()( T const &value )
    {
        str = boost::lexical_cast< string >( *value );
    }
};

void ParamUI::Param::setFromString( const string &string )
{
    try
    {
        boost::apply_visitor( from_string_visitor( string ), mPtr );
    }

    catch ( boost::bad_lexical_cast err )
    {
        CI_LOG_W( "Could not set param " << mName << " with value of " << string );
    }
}

string ParamUI::Param::getString()
{
    string str = "";
    try
    {
        to_string_visitor visitor;
        boost::apply_visitor( visitor, mPtr );
        str = visitor.str;
    }
    catch ( boost::bad_lexical_cast err )
    {
        CI_LOG_W( "Could not get string from param " << mName );
    }

    return str;
}

#pragma mark -- ParamUI

ParamUI::ParamUI()
{
    mServer.getEventSignal( Event::Type::SET ).connect( ::std::bind( &ParamUI::onSet, this, ::std::placeholders::_1 ) );

    mServer.getEventSignal( Event::Type::GET ).connect( ::std::bind( &ParamUI::onGet, this, ::std::placeholders::_1 ) );
}

ParamUI::ParamOptions & ParamUI::bind( const string &name, float *floatParam )
{
    mParams.emplace( name, Param( name, floatParam ) );
    return mParams.at( name ).getOptions();
}

ParamUI::ParamContainer::iterator ParamUI::findParam( const string &name )
{
    return mParams.find( name );
}

void ParamUI::onSet( Event event )
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

        it->second.setFromString( n.getValue() );
    }
}

void ParamUI::onGet( Event event )
{
    string name = event.getData().getValue();
    auto it = findParam( name );
    if ( it == mParams.end() )
    {
        CI_LOG_W( "unknown param: " << name );
        return;
    }

    string value = it->second.getString();
    mServer.set( it->first, value );
}