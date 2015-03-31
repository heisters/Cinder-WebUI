#include "WebUI.h"
#include "cinder/Log.h"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

using namespace ci;
using namespace std;

#pragma mark -- WebUI::Event

WebUI::Event::Event() :
mIsValid( false ),
mType( Type::UNKNOWN )
{

}

WebUI::Event::Event( const Type &type, const std::string &name, const std::string &value ) :
mIsValid( true ),
mType( type ),
mName( name ),
mValue( value )
{

}

#pragma mark -- WebUI

WebUI::WebUI()
{
    mServer.addConnectCallback( &WebUI::onConnect, this );
    mServer.addDisconnectCallback( &WebUI::onDisconnect, this );
    mServer.addErrorCallback( &WebUI::onError, this );
    mServer.addInterruptCallback( &WebUI::onInterrupt, this );
    mServer.addPingCallback( &WebUI::onPing, this );
    mServer.addReadCallback( &WebUI::onRead, this );
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

void WebUI::write( const string &msg )
{
    mServer.write( msg );
}

void WebUI::onConnect()
{
    CI_LOG_I( "connect" );
}

void WebUI::onDisconnect()
{
    CI_LOG_I( "disconnect" );
}

void WebUI::onInterrupt()
{
    CI_LOG_I( "interrupt" );
}

void WebUI::onError( string err )
{
    CI_LOG_I( "error: " << err );
}

void WebUI::onPing( string msg )
{
    CI_LOG_V( "ping: " << msg );
}

void WebUI::onRead( string msg )
{
    CI_LOG_V( "read: " << msg );

    dispatch( parse( msg ) );
    
}

WebUI::Event WebUI::parse( const string &msg )
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

void WebUI::dispatch( const WebUI::Event &event )
{
    switch ( event.getType() ) {
        case Event::Type::SET:
            getSetSignal()( event );
            break;

        default:
            break;
    }
}

#pragma mark -- WebParamUI::Param

WebParamUI::Param::Param( const string &name, float *ptr ) :
mName( name ),
mPtr( ptr )
{

}

WebParamUI::ParamOptions & WebParamUI::Param::getOptions()
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

void WebParamUI::Param::setFromString( const string &string )
{
    boost::apply_visitor( from_string_visitor( string ), mPtr );
}

#pragma mark -- WebParamUI

WebParamUI::WebParamUI()
{
    using namespace placeholders;
    getSetSignal().connect( bind( &WebParamUI::onSet, this, ::_1 ) );
}

WebParamUI::ParamOptions & WebParamUI::addParam( const string &name, float *floatParam )
{
    mParams.emplace( name, Param( name, floatParam ) );
    return mParams.at( name ).getOptions();
}

WebParamUI::Param & WebParamUI::getParam( const string &name )
{
    return mParams.at( name );
}

WebParamUI::ParamContainer::iterator WebParamUI::findParam( const string &name )
{
    return mParams.find( name );
}

void WebParamUI::onSet( WebUI::Event event )
{
    auto it = findParam( event.getName() );
    if ( it == mParams.end() )
    {
        CI_LOG_W( "unknown param: " << event.getName() );
        return;
    }

    it->second.setFromString( event.getValue() );
}