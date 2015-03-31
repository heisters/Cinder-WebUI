#include "WebUI.h"
#include "cinder/Log.h"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

using namespace ci;
using namespace std;
using namespace webui;

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

    dispatch( parse( msg ) );

}

Event Server::parse( const string &msg )
{
    vector< string > tokens;
    boost::split( tokens, msg, boost::is_any_of( " " ) );
    if ( tokens.size() != 3 )
    {
        CI_LOG_W( "invalid message received: " << msg );
        return Event();
    }

    string str_type = tokens.at( 0 );
    Event::Type type = Event::Type::UNKNOWN;
    if ( str_type == "set" ) type = Event::Type::SET;
    else
    {
        CI_LOG_W( "unrecognized message type: " << msg );
        return Event();
    }

    return Event( type, tokens.at( 1 ), tokens.at( 2 ) );
}

void Server::dispatch( const Event &event )
{
    switch ( event.getType() ) {
        case Event::Type::SET:
            getSetSignal()( event );
            break;

        default:
            break;
    }
}


#pragma mark -- Event

Event::Event() :
mType( Type::UNKNOWN )
{

}

Event::Event( const Type &type, const std::string &name, const std::string &value ) :
mType( type ),
mName( name ),
mValue( value )
{

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

void BaseUI::write( const string &msg )
{
    mServer.write( msg );
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
    from_string_visitor( const string &str ) : str( str ) {}

    template< typename T >
    void operator()( T const &value ) const
    {
        auto v = *value; // convert to rvalue: "if the value category of expression is lvalue, then the decltype specifies T&"
        *value = boost::lexical_cast< decltype( v ) >( str );
    }

    string str;
};

void ParamUI::Param::setFromString( const string &string )
{
    boost::apply_visitor( from_string_visitor( string ), mPtr );
}

#pragma mark -- ParamUI

ParamUI::ParamUI()
{
    using namespace placeholders;
    mServer.getSetSignal().connect( bind( &ParamUI::onSet, this, ::_1 ) );
}

ParamUI::ParamOptions & ParamUI::addParam( const string &name, float *floatParam )
{
    mParams.emplace( name, Param( name, floatParam ) );
    return mParams.at( name ).getOptions();
}

ParamUI::Param & ParamUI::getParam( const string &name )
{
    return mParams.at( name );
}

ParamUI::ParamContainer::iterator ParamUI::findParam( const string &name )
{
    return mParams.find( name );
}

void ParamUI::onSet( Event event )
{
    auto it = findParam( event.getName() );
    if ( it == mParams.end() )
    {
        CI_LOG_W( "unknown param: " << event.getName() );
        return;
    }

    it->second.setFromString( event.getValue() );
}