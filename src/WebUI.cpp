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
    set( name, vector< float >{ value.x, value.y } );
}

void Server::set( const string &name, const vec3 &value )
{
    set( name, vector< float >{ value.x, value.y, value.z } );
}

void Server::set( const std::string &name, const ci::Colorf &value )
{
    set( name, vector< float >{ value.r, value.g, value.b } );
}

template< typename T >
void Server::set( const std::string &name, const vector< T > &value )
{
    JsonTree json = JsonTree::makeArray( name );
    for ( const auto &v : value ) json.addChild( JsonTree( "", v ) );

    stringstream ss;
    ss << makeSetJSON( json );
    write( ss.str() );
}

template< typename T, typename U >
void Server::set( const std::string &name, const map< T, U > &value )
{
    JsonTree json = JsonTree::makeObject( name );
    for ( const auto &v : value ) json.addChild( JsonTree( v.first, v.second ) );

    stringstream ss;
    ss << makeSetJSON( json );
    write( ss.str() );
}


void Server::dispatch( const Event &event )
{
    getEventSignal( event.getType() )( event );
}


#pragma mark -- WebUI

WebUI::WebUI()
{
	mServer.connectOpenEventHandler([&]()
	{
		CI_LOG_I("Client connected");
	});
	mServer.connectCloseEventHandler([&]()
	{
		CI_LOG_I("Client disconnected");
	});
	mServer.connectFailEventHandler([&](string err)
	{
		CI_LOG_W("WebUI error: " << err);
	});
	mServer.connectInterruptEventHandler([&]()
	{
		CI_LOG_I("interrupt");
	});
	mServer.connectPingEventHandler([&](string msg)
	{
		CI_LOG_I("ping");
	});
	mServer.connectMessageEventHandler([&](string msg)
	{
		JsonTree parsed;
		try
		{
			parsed = JsonTree(msg);
		}
		catch (JsonTree::ExcJsonParserError err) {
			CI_LOG_W("Could not parse JSON: " << msg << " " << err.what());
			return;
		}

		for (const auto &n : parsed)
		{
			string command = n.getKey();
			if (command == "set")
			{
				CI_LOG_V("WebUI received set: " << parsed);
				mServer.dispatch(Event(Event::Type::SET, parsed.getChild("set")));
			}

			else if (command == "select")
			{
				CI_LOG_V("WebUI received select: " << parsed);
				mServer.dispatch(Event(Event::Type::SELECT, parsed.getChild("select")));
			}

			else if (command == "get")
			{
				CI_LOG_V("WebUI received get: " << parsed);
				mServer.dispatch(Event(Event::Type::GET, parsed.getChild("get")));
			}
		}
	});

    mServer.getEventSignal( Event::Type::SET ).connect( ::std::bind( &WebUI::onRemoteSet, this, ::std::placeholders::_1 ) );
    mServer.getEventSignal( Event::Type::SELECT ).connect( ::std::bind( &WebUI::onRemoteSelect, this, ::std::placeholders::_1 ) );

    mServer.getEventSignal( Event::Type::GET ).connect( ::std::bind( &WebUI::onRemoteGet, this, ::std::placeholders::_1 ) );
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

vec2 jsonToVec2( const JsonTree &json )
{
    return vec2(json.getChild( 0 ).getValue< float >(),
                json.getChild( 1 ).getValue< float >());
}

vec3 jsonToVec3( const JsonTree &json )
{
    return vec3(json.getChild( 0 ).getValue< float >(),
                json.getChild( 1 ).getValue< float >(),
                json.getChild( 2 ).getValue< float >());
}

Colorf jsonToColorf( const JsonTree &json )
{
    return Colorf(json.getChild( 0 ).getValue< float >(),
                  json.getChild( 1 ).getValue< float >(),
                  json.getChild( 2 ).getValue< float >());
}

struct set_from_json_visitor : boost::static_visitor<>
{
    set_from_json_visitor( const JsonTree &j ) : json( j ) {};
    JsonTree json;

    template< typename T >
    void operator()( T const &param_ptr ) const
    {
        auto v = (*param_ptr)();
        param_ptr->set( json.getValue< decltype( v ) >(), remote );
    }

    void operator()( BoundParam< vec2 >* const &param_ptr ) const
    {
        param_ptr->set( jsonToVec2( json ), remote );
    }

    void operator()( BoundParam< vec3 >* const &param_ptr ) const
    {
        param_ptr->set( jsonToVec3( json ), remote );
    }

    void operator()( BoundParam< Colorf >* const &param_ptr ) const
    {
        param_ptr->set( jsonToColorf( json ), remote );
    }

    void operator()( BoundParam< vector< string > >* const &param_ptr ) const
    {
        param_ptr->clear();
        for ( const auto &c : json.getChildren() )
        {
            param_ptr->push_back( c.getValue< string >(), remote );
        }
    }

    void operator()( BoundParam< map< string, string > >* const &param_ptr ) const
    {
        for ( const auto &c: json.getChildren() )
        {
            param_ptr->set( make_pair( c.getKey(), c.getValue< string >() ), remote );
        }
    }
};

struct select_from_json_visitor : boost::static_visitor<>
{
    select_from_json_visitor( const JsonTree &j ) : json( j ) {};
    JsonTree json;

    template< typename T >
    void operator()( T const &param_ptr ) const
    {
        auto v = (*param_ptr)();
        param_ptr->select( json.getValue< decltype( v ) >(), remote );
    }

    void operator()( BoundParam< vec2 >* const &param_ptr ) const
    {
        param_ptr->select( jsonToVec2( json ), remote );
    }

    void operator()( BoundParam< vec3 >* const &param_ptr ) const
    {
        param_ptr->select( jsonToVec3( json ), remote );
    }

    void operator()( BoundParam< Colorf >* const &param_ptr ) const
    {
        param_ptr->select( jsonToColorf( json ), remote );
    }

    void operator()( BoundParam< vector< string > >* const &param_ptr ) const
    {
        param_ptr->select( json.getValue< string >(), remote );
    }

    void operator()( BoundParam< map< string, string > >* const &param_ptr ) const
    {
        param_ptr->select( make_pair( json.getKey(), json.getValue< string >() ), remote );
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

void WebUI::onRemoteSet( Event event )
{
    for ( const auto &n : event.getData() )
    {
        string name = n.getKey();
        auto it = findParam( name );
        if ( it == mParams.end() ) continue;

        try
        {
            boost::apply_visitor( set_from_json_visitor( n ), it->second );
        }

        catch ( JsonTree::Exception err )
        {
            CI_LOG_W( "Could not set param " << it->first << " with value value of " << n << ". " << err.what() );
        }
    }
}

void WebUI::onRemoteSelect( Event event )
{
    for ( const auto &n : event.getData() )
    {
        string name = n.getKey();
        auto it = findParam( name );
        if ( it == mParams.end() ) continue;

        try
        {
            boost::apply_visitor( select_from_json_visitor( n ), it->second );
        }

        catch ( JsonTree::Exception err )
        {
            CI_LOG_W( "Could not select param " << it->first << " with value value of " << n << ". " << err.what() );
        }
    }
}

void WebUI::onRemoteGet( Event event )
{
    string name = event.getData().getValue();
    auto it = findParam( name );
    if ( it == mParams.end() ) return;

    server_set_visitor visitor( mServer, it->first );
    boost::apply_visitor( visitor, it->second );
}

void WebUI::onLocalSet( source s, const string &name )
{
    if ( s != local ) return;

    auto it = findParam( name );
    if ( it == mParams.end() ) return;

    server_set_visitor visitor( mServer, it->first );
    boost::apply_visitor( visitor, it->second );
}