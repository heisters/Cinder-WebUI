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
    enum class Type { UNKNOWN, GET, SET, SELECT };
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
    void                        set( const std::string &name, const ci::Colorf &value );
    template< typename T >
    void                        set( const std::string &name, const std::vector< T > &value );

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

template< typename T > struct contained_type { typedef T value; };
template< typename U > struct contained_type< std::vector< U > > { typedef U value; };

template< typename T >
class BoundParam
{
public:
    typedef T value;
    typedef typename contained_type< T >::value contained_value;
    typedef boost::signals2::signal< void ( value ) > signal;
    typedef boost::signals2::signal< void ( contained_value ) > signal_select;
    typedef boost::signals2::signal< void ( ci::JsonTree ) > signal_json;

    BoundParam() {};
    BoundParam( const T &v ) : mValue( v ) {};

    // no "T&" methods are provided, so that you can't accidentally modify the
    // value without notifying of change

    T                           get() const { return mValue; }
    T                           set( const T & v, bool notify=true ) { mValue = v; if(notify) notifyChange(); return get(); }
    template< typename U = T >
    void                        push_back( const typename U::value_type &v, bool notify=true ) { mValue.push_back( v ); if (notify) notifyChange(); }
    
    operator                    T () const { return get(); }
    T operator                  () () const { return get(); }
    T operator                  = ( const T & v ) { return set( v ); }
    T operator                  += ( const T & v ) { return set( mValue + v ); }
    T operator                  -= ( const T & v ) { return set( mValue - v ); }
    T operator                  ++ () { mValue++; notifyChange(); return get(); }
    T operator                  -- () { mValue--; notifyChange(); return get(); }

    signal &                    getChangeSignal() { return mChangeSignal; }
    signal_select &             getSelectSignal() { return mSelectSignal; }
    signal_json &               getSelectJSONSignal() { return mSelectJSONSignal; }

    void                        select( const ci::JsonTree &json ) { getSelectJSONSignal()( json ); }
    void                        select( const contained_value &v ) { getSelectSignal()( v ); }
private:
    void                        notifyChange() { getChangeSignal()( get() ); }

    T                           mValue;
    signal                      mChangeSignal;
    signal_select               mSelectSignal;
    signal_json                 mSelectJSONSignal;
};

////////////////////////////////////////////////////////////////////////////////
// WebUI
class WebUI
{
public:
    WebUI();

    void                            update();
    void                            listen( uint16_t port );

    typedef boost::variant< BoundParam< bool >*, BoundParam< int >*, BoundParam< float >*, BoundParam< glm::vec2 >*, BoundParam< glm::vec3 >*, BoundParam< std::string >*, BoundParam< double >*, BoundParam< ci::Colorf >*, BoundParam< std::vector< std::string > >* > bound_param_ptr;
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
    void                            onSelect( Event event );
    void                            onParamChange( const std::string &name );

    bound_params_container          mParams;

    Server                          mServer;
};

} // end namespace webui