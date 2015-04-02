#pragma once

#include "WebSocketServer.h"
#include "cinder/Json.h"
#include "boost/variant.hpp"
#include "boost/signals2.hpp"
#include <map>

namespace webui {

////////////////////////////////////////////////////////////////////////////////
// Event
class Event
{
public:
    enum class Type { UNKNOWN, GET, SET };
    Event();
    Event( const Type &type, const ci::JsonTree &data );

    Type                    getType() const { return mType; }
    const ci::JsonTree &    getData() const { return mData; }
    ci::JsonTree            getData() { return mData; }
private:
    Type                    mType;
    ci::JsonTree            mData;
};

////////////////////////////////////////////////////////////////////////////////
// Server
class Server : public WebSocketServer
{
public:
    typedef boost::signals2::signal< void ( Event ) > event_signal;
    typedef std::map< Event::Type, event_signal > event_signal_container;

    Server();

    event_signal &              getEventSignal( const Event::Type &type );

    void                        get( const std::string &name );
    template< typename T >
    void                        set( const std::string &name, const T &value );
    void                        set( const std::string &name, const glm::vec2 &value );
    void                        set( const std::string &name, const glm::vec3 &value );

private:
    void						onConnect();
    void						onDisconnect();
    void						onError( std::string err );
    void						onInterrupt();
    void						onPing( std::string msg );
    void						onRead( std::string msg );

    event_signal_container      mEventSignals;
    void                        dispatch( const Event &event );
};

////////////////////////////////////////////////////////////////////////////////
// BoundParam
template< typename T >
class BoundParam
{
public:
    typedef T value;
    typedef boost::signals2::signal< void ( T ) > signal;

    BoundParam() {};
    BoundParam( const T &v ) : mValue( v ) {};

    T                           get() const { return mValue; }
    T                           set( const T & v, bool notify=true ) { mValue = v; if(notify) notifyChange(); return get(); }
    operator                    T () const { return get(); }
    T operator                  () () const { return get(); }
    T operator                  = ( const T & v ) { return set( v ); }
    T operator                  += ( const T & v ) { return set( mValue + v ); }
    T operator                  -= ( const T & v ) { return set( mValue - v ); }
    T operator                  ++ () { mValue++; notifyChange(); return get(); }
    T operator                  -- () { mValue--; notifyChange(); return get(); }

    signal &                    getChangeSignal() { return mChangeSignal; }
private:
    void                        notifyChange() { getChangeSignal()( get() ); }

    T                           mValue;
    signal                      mChangeSignal;
};

////////////////////////////////////////////////////////////////////////////////
// WebUI
class WebUI
{
public:
    WebUI();

    void                            update();
    void                            listen( uint16_t port );

    typedef boost::variant< BoundParam< int >*, BoundParam< float >*, BoundParam< glm::vec2 >*, BoundParam< glm::vec3 >*, BoundParam< std::string >* > bound_param_ptr;
    typedef std::map< std::string, bound_param_ptr > bound_params_container;

    template< typename T >
    void                            bind( const std::string &name, T *param )
    {
        param->getChangeSignal().connect( std::bind( &WebUI::onParamChange, this, name ) );
        mParams.insert( make_pair( name, param ) );
    }

private:

    bound_params_container::iterator findParam( const std::string &name );
    void                            onSet( Event event );
    void                            onGet( Event event );
    void                            onParamChange( const std::string &name );
    void                            setClients( const bound_params_container::value_type &pair );
    void                            setSelf( const bound_params_container::value_type &pair, const ci::JsonTree &value );

    bound_params_container          mParams;

    Server                          mServer;
};

} // end namespace webui